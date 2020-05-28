/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* extstore.c
 * KVSNS: implement a dummy object store inside a POSIX directory
 */


#include <hiredis/hiredis.h>
#include <kvsns/extstore.h>

#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc != 0)	\
		return __rc; })

#define RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	if (__rc != 0)        \
		goto __label; })


/* The REDIS context exists in the TLS, for MT-Safety */
__thread redisContext *rediscontext = NULL;

static char store_root[MAXPATHLEN];

static struct collection_item *conf = NULL;

static void extstore_reinit(void)
{
	extstore_init(conf);
}

enum state {
	INVAL      = -1,
	CACHED     = 0,	/* Cache is newer than  Object Store  */
	DUPLICATED = 1,	/* Cache is the same as Object Store  */
	RELEASED   = 2,	/* In cache, not in Object Store      */
	STATE_LAST = 3,	/* For iteration only */
};

static const char * const state_names[] = {
	[CACHED] = "CACHED",
	[DUPLICATED]  = "DUPLICATED",
	[RELEASED] = "RELEASED",
};

static const char *state2str(enum state state)
{
	if (state >= STATE_LAST || state < 0)
		return NULL;
	return state_names[state];
}

static enum state str2state(const char *str)
{
	int i;

	for (i = 0; i < STATE_LAST; i++)
		if (!strcmp(str, state_names[i]))
			return i;
	return INVAL;
}


static int setkey(char *k, char *v)
{
	redisReply *reply = NULL;

	if (!k || !v)
		return -EINVAL;

	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	return 0;
}

static int getkey(char *k, char *v)
{
	redisReply *reply;

	if (!k || !v)
		return -EINVAL;

	reply = NULL;
	reply = redisCommand(rediscontext, "GET %s", k);
	if (!reply)
		return -1;

	if (reply->len == 0)
		return -ENOENT;

	strcpy(v, reply->str);
	freeReplyObject(reply);

	return 0;
}

static int delkey(char *k)
{
	redisReply *reply = NULL;

	if (!k)
		return -EINVAL;

	reply = redisCommand(rediscontext, "DEL %s", k);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	return 0;
}

static int setkeybuf(char *k, char *v, size_t len)
{
	redisReply *reply = NULL;

	if (!k || !v)
		return -EINVAL;

	reply = redisCommand(rediscontext, "SET %s %b", k, v, len);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	return 0;
}

static int get_entry_state(kvsns_ino_t *ino, enum state *state)
{
	char k[KLEN];
	char v[VLEN];

	snprintf(k, KLEN, "%llu.cache_state", *ino);
	RC_WRAP(getkey, k, v);

	*state = str2state(v);

	return 0;
}

static int set_entry_state(kvsns_ino_t *ino, enum state state)
{
	char  k[KLEN];
	char *v = NULL;

	snprintf(k, KLEN, "%llu.cache_state", *ino);
	v = (char *)state2str(state);

	RC_WRAP(setkey, k, v);

	return 0;
}

static int del_entry_state(kvsns_ino_t *ino)
{
	return -ENOTSUP;
}

static int set_stat(kvsns_ino_t *ino, struct stat *buf)
{
	char k[KLEN];

	size_t size = sizeof(struct stat);

	if (!ino || !buf)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.data_attr", *ino);
	RC_WRAP(setkeybuf, k, (char *)buf, size);

	return 0;
}

static int get_stat(kvsns_ino_t *ino, struct stat *buf)
{
	char k[KLEN];
	redisReply *reply;

	if (!ino || !buf)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.data_attr", *ino);
	reply = redisCommand(rediscontext, "GET %s", k);
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STRING)
		return -1;

	if (reply->len != sizeof(struct stat))
		return -1;

	memcpy((char *)buf, reply->str, reply->len);

	freeReplyObject(reply);

	return 0;
}

static int build_extstore_path(kvsns_ino_t object,
			       char *extstore_path,
			       size_t pathlen)
{
	char k[KLEN];

	if (!extstore_path)
		return -1;

	if (!rediscontext)
		extstore_reinit();

	snprintf(k, KLEN, "%llu.data", object);
	RC_WRAP(getkey, k, extstore_path);

	return 0;
}

enum update_stat_how {
	UP_ST_WRITE = 1,
	UP_ST_READ = 2,
	UP_ST_TRUNCATE = 3
};

static int update_stat(struct stat *stat, enum update_stat_how how,
		       off_t size)
{
	struct timeval t;

	if (!stat)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	switch (how) {
	case UP_ST_WRITE:
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
		stat->st_ctim = stat->st_mtim;
		if (size > stat->st_size) {
			stat->st_size = size;
			stat->st_blocks = size / DEV_BSIZE;
		}
		break;

	case UP_ST_READ:
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
		break;

	case UP_ST_TRUNCATE:
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
		stat->st_mtim = stat->st_ctim;
		stat->st_size = size;
		stat->st_blocks = size / DEV_BSIZE;
		break;

	default: /* Should not occur */
		return -EINVAL;
		break;
	}

	return 0;
}
int extstore_create(kvsns_ino_t object)
{
	char k[KLEN];
	char v[VLEN];
	char path[VLEN];
	int fd;
	size_t size;
	struct stat stat;

	if (!rediscontext)
		extstore_reinit();

	if (!rediscontext)
		extstore_reinit();

	fd = creat(path, 0777);
	if (fd == -1)
		return -errno;
	close(fd);

	snprintf(k, KLEN, "%llu.data", object);
	snprintf(path, VLEN, "%s/inum=%llu",
		store_root, (unsigned long long)object);
	strncpy(v, path, VLEN);
	RC_WRAP(setkey, k, v);

	snprintf(k, KLEN, "%llu.data_attr", object);
	size = sizeof(struct stat);
	memset((char *)&stat, 0, sizeof(stat));
	RC_WRAP(setkeybuf, k, (char *)&stat, size);

	snprintf(k, KLEN, "%llu.data_ext", object);
	v[0] = '\0'; /* snprintf(v, VLEN, ""); */
	RC_WRAP(setkey, k, v);

	snprintf(k, KLEN, "%llu.cache_state", object);
	snprintf(v, VLEN, "1/0");
	RC_WRAP(setkey, k, v);

	return 0;
}

int extstore_attach(kvsns_ino_t *ino, char *objid, int objid_len)
{
	char k[KLEN];
	char v[VLEN];
	size_t size;
	struct stat stat;

	if (!rediscontext)
		extstore_reinit();

	if (!rediscontext)
		extstore_reinit();

	snprintf(k, KLEN, "%llu.data", *ino);
	strncpy(v, objid, (objid_len > VLEN)?VLEN:objid_len);
	RC_WRAP(setkey, k, v);

	snprintf(k, KLEN, "%llu.data_attr", *ino);
	size = sizeof(struct stat);
	memset((char *)&stat, 0, sizeof(stat));
	RC_WRAP(setkeybuf, k, (char *)&stat, size);

	snprintf(k, KLEN, "%llu.data_ext", *ino);
	v[0] = '\0'; /* snprintf(v, VLEN, ""); */
	RC_WRAP(setkey, k, v);

	snprintf(k, KLEN, "%llu.cache_state", *ino);
	snprintf(v, VLEN, "YES");
	RC_WRAP(setkey, k, v);

	RC_WRAP(set_entry_state, ino, CACHED);

	return 0;
}

int extstore_init(struct collection_item *cfg_items)
{
	redisReply *reply;
	char *hostname = NULL;
	char hostname_default[] = "127.0.0.1";
	struct timeval timeout = { 1, 500000 }; /* 1.5 seconds */
	int port = 6379; /* REDIS default */
	struct collection_item *item;
	int rc;

	if (cfg_items != NULL)
		conf = cfg_items;

	/* Get config from ini file */
	item = NULL;
	RC_WRAP(get_config_item, "posix_obj", "server", cfg_items, &item);
	if (item == NULL)
		hostname = hostname_default;
	else
		hostname = get_string_config_value(item, NULL);

	item = NULL;
	RC_WRAP(get_config_item, "posix_obj", "port", cfg_items, &item);
	if (item != NULL)
		port = (int)get_int_config_value(item, 0, 0, NULL);

	rediscontext = redisConnectWithTimeout(hostname, port, timeout);
	if (rediscontext == NULL || rediscontext->err) {
		if (rediscontext) {
			fprintf(stderr,
				"Connection error: %s\n", rediscontext->errstr);
			redisFree(rediscontext);
		} else {
			fprintf(stderr,
				"Connection error: can't get redis context\n");
		}
		exit(1);
	}

	/* PING server */
	reply = redisCommand(rediscontext, "PING");
	if (!reply)
		return -1;

	freeReplyObject(reply);

	/* Deal with store_root */
	item = NULL;
	rc = get_config_item("posix_obj", "root_path",
			      cfg_items, &item);
	if (rc != 0)
		return -rc;
	if (item == NULL)
		return -EINVAL;

	strncpy(store_root, get_string_config_value(item, NULL),
		MAXPATHLEN);

	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	char k[KLEN];
	char storepath[MAXPATHLEN];
	int rc;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc) {
		if (rc == -ENOENT) /* No data created */
			return 0;

		return rc;
	}

	rc = unlink(storepath);
	if (rc) {
		if (errno == ENOENT)
			return 0;

		return -errno;
	}

	/* delete <inode>.data */
	snprintf(k, KLEN, "%llu.data", *ino);
	RC_WRAP(delkey, k);

	/* delete <inode>.data_attr */
	snprintf(k, KLEN, "%llu.data_attr", *ino);
	RC_WRAP(delkey, k);

	/* delete <inode>.data_ext */
	snprintf(k, KLEN, "%llu.data_ext", *ino);
	RC_WRAP(delkey, k);

	/* delete state */
	RC_WRAP(del_entry_state, ino);

	return 0;
}

int extstore_read(kvsns_ino_t *ino,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc = 0;
	int fd = 0;
	ssize_t read_bytes;
	enum state state;

	RC_WRAP(get_entry_state, ino, &state);

	if (state == RELEASED)
		RC_WRAP(extstore_restore, ino);

	RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);

	fd = open(storepath, O_CREAT|O_RDONLY|O_SYNC, 0755);
	if (fd < 0)
		return -errno;

	read_bytes = pread(fd, buffer, buffer_size, offset);
	if (read_bytes < 0) {
		close(fd);
		return -errno;
	}

	RC_WRAP_LABEL(rc, errout, update_stat, stat, UP_ST_READ, 0);
	RC_WRAP_LABEL(rc, errout, set_stat, ino, stat);

	rc = close(fd);
	if (rc < 0)
		return -errno;

	return read_bytes;

errout:
	close(fd);

	return rc;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc;
	int fd;
	ssize_t written_bytes;
	struct stat objstat;
	enum state state;

	if (!ino)
		return -EINVAL;

	RC_WRAP(get_entry_state, ino, &state);

	if (state == RELEASED)
		RC_WRAP(extstore_restore, ino);

	RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);

	fd = open(storepath, O_CREAT|O_WRONLY|O_SYNC, 0755);
	if (fd < 0)
		return -errno;

	written_bytes = pwrite(fd, buffer, buffer_size, offset);
	if (written_bytes < 0) {
		close(fd);
		return -errno;
	}

	rc = close(fd);
	if (rc < 0)
		return -errno;

	RC_WRAP(get_stat, ino, &objstat);
	RC_WRAP(update_stat, &objstat, UP_ST_WRITE,
		offset+written_bytes);
	RC_WRAP(set_stat, ino, &objstat);

	stat->st_size = objstat.st_size;
	stat->st_blocks = objstat.st_blocks;
	stat->st_mtim = objstat.st_mtim;
	stat->st_ctim = objstat.st_ctim;

	if (state != CACHED)
		RC_WRAP(set_entry_state, ino, CACHED);

	*fsal_stable = true;
	return written_bytes;
}


int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      bool running_attach,
		      struct stat *stat)
{
	int rc;
	char storepath[MAXPATHLEN];
	struct stat objstat;
	enum state state;

	if (!ino || !stat)
		return -EINVAL;

	RC_WRAP(get_entry_state, ino, &state);

	switch (state) {
	case CACHED:
		RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);
		RC_WRAP(truncate, storepath, filesize);
		rc = 0;
		break;
	case DUPLICATED:
		RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);
		RC_WRAP(truncate, storepath, filesize);
		RC_WRAP(set_entry_state, ino, CACHED);
		rc = 0;
		break;
	case RELEASED:
		RC_WRAP(get_stat, ino, &objstat);

		stat->st_size = filesize;
		objstat.st_size = filesize;
		RC_WRAP(update_stat, &objstat, UP_ST_TRUNCATE, filesize);
		stat->st_ctim = objstat.st_ctim;
		stat->st_mtim = objstat.st_mtim;

		RC_WRAP(set_stat, ino, &objstat);
		rc = 0;
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_getattr(kvsns_ino_t *ino,
		     struct stat *stat)
{
	int rc;
	char storepath[MAXPATHLEN];
	enum state state;

	/* Should be using stored stat */
	if (!ino || !stat)
		return -EINVAL;

	RC_WRAP(get_entry_state, ino, &state);

	switch (state) {
	case CACHED:
	case DUPLICATED:
		/* Newer data are in cache, let's query it */
		RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);
		RC_WRAP(lstat, storepath, stat);
		rc = 0; /* Success */
		break;
	case RELEASED:
		/* Get archived stats */
		RC_WRAP(get_stat, ino, stat);
		rc = 0; /* Success */
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_archive(kvsns_ino_t *ino)
{
	return -ENOTSUP;
}

int extstore_restore(kvsns_ino_t *ino)
{
	return -ENOTSUP;
}

int extstore_release(kvsns_ino_t *ino)
{
	return -ENOTSUP;
}

int extstore_state(kvsns_ino_t *ino, char *strstate)
{
	enum state state;

	RC_WRAP(get_entry_state, ino, &state);
	strcpy(strstate, state2str(state));

	return 0;
}

