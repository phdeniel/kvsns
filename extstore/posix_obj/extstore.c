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
	if (__rc != 0)        \
		return __rc; })

/* The REDIS context exists in the TLS, for MT-Safety */
__thread redisContext *rediscontext = NULL;

static char kvsns_store_default[] = KVSNS_STORE_DEFAULT;
static char kvsns_store_base[MAXPATHLEN];
static int kvsns_debug = false;

static char store_root[MAXPATHLEN];

static int build_extstore_path(kvsns_ino_t object,
			       char *extstore_path,
			       size_t pathlen)
{
	char k[KLEN];
	redisReply *reply;

	if (!extstore_path)
		return -1;

	snprintf(k, KLEN, "%llu.data", object);	
	reply = NULL;
	reply = redisCommand(rediscontext, "GET %s", k);
	if (!reply)
		return -1;

	if(reply->len == 0)
		return -ENOENT;

	strcpy(extstore_path, reply->str);
	freeReplyObject(reply);
	return 0;
}

enum update_stat_how {
	UP_ST_WRITE = 1,
	UP_ST_READ = 2,
	UP_ST_TRUNCATE =3
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

int extstore_create(kvsns_ino_t object, struct stat *stat)
{
	char k[KLEN];
	char v[VLEN];
	char path[VLEN];
	redisReply *reply;
	int fd;
	size_t size;
	struct timeval t;

	snprintf(k, KLEN, "%llu.data", object);
	snprintf(path, VLEN, "%s/inum=%llu",
		store_root, (unsigned long long)object);
	strncpy(v, path, VLEN);

	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	/* Create initial attrs */
	if (gettimeofday(&t, NULL) != 0)
		return -errno;
	
	snprintf(k, KLEN, "%llu.data_attr", object);
	size = sizeof(struct stat);

	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %b", k, stat, size);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	snprintf(k, KLEN, "%llu.data_ext", object);
	snprintf(v, VLEN, "");
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;

	freeReplyObject(reply);
	fd = creat(path, 0777);
	if (fd == -1)
		return -errno;

	close(fd);
	return 0;
}

static int set_stat(kvsns_ino_t *ino, struct stat *buf)
{
        redisReply *reply;
	char k[KLEN];

        size_t size = sizeof(struct stat);

        if (!ino || !buf)
                return -EINVAL;

	snprintf(k, KLEN, "%llu.data_attr", *ino);
        reply = redisCommand(rediscontext, "SET %s %b", k, buf, size);
        if (!reply)
                return -1;

	freeReplyObject(reply);
        return 0;
}

static int get_stat(kvsns_ino_t *ino, struct stat *buf)
{
        redisReply *reply;
        char k[KLEN];
        int rc;

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

int extstore_init(char *rootpath)
{
        redisReply *reply;
        char *hostname = NULL;
        char hostname_default[] = "127.0.0.1";
        struct timeval timeout = { 1, 500000 }; /* 1.5 seconds */
        int port = 6379; /* REDIS default */

        hostname = getenv(KVSNS_SERVER);
        if (hostname == NULL)
                hostname = hostname_default;

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

	strncpy(store_root, rootpath, MAXPATHLEN);
	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	char k[KLEN];
	char storepath[MAXPATHLEN];
	int rc;
        redisReply *reply;

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
	reply = NULL;
	reply = redisCommand(rediscontext, "DEL %s", k);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	/* delete <inode>.data_attr */	
	snprintf(k, KLEN, "%llu.data_attr", *ino);	
	reply = NULL;
	reply = redisCommand(rediscontext, "DEL %s", k);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	/* delete <inode>.data_ext */	
	snprintf(k, KLEN, "%llu.data_ext", *ino);	
	reply = NULL;
	reply = redisCommand(rediscontext, "DEL %s", k);
	if (!reply)
		return -1;
	freeReplyObject(reply);

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
	int rc;
	int fd;
	ssize_t read_bytes;

	RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);

	fd = open(storepath, O_CREAT|O_RDONLY|O_SYNC);
	if (fd < 0) {
		return -errno;
	}

	read_bytes = pread(fd, buffer, buffer_size, offset);
	if (read_bytes < 0) {
		close(fd);
		return -errno;
	}

	RC_WRAP(update_stat, stat, UP_ST_READ, 0);
	RC_WRAP(set_stat, ino, stat);

	rc = close(fd);
	if (rc < 0)
		return -errno;


	return read_bytes;
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

	*fsal_stable = true;
	return written_bytes;
}


int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      struct stat *stat)
{
	int rc;
	char storepath[MAXPATHLEN];
	struct stat objstat;

	if (!ino || !stat)
		return -EINVAL;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	RC_WRAP(get_stat, ino, &objstat);

	rc = truncate(storepath, filesize);
	if (rc < 0)
		return -errno;

	stat->st_size = filesize;
	objstat.st_size = filesize;

	RC_WRAP(update_stat, &objstat, UP_ST_TRUNCATE, filesize);

	stat->st_ctim = objstat.st_ctim;
	stat->st_mtim = objstat.st_mtim;

	RC_WRAP(set_stat, ino, &objstat);

	return 0;
}

