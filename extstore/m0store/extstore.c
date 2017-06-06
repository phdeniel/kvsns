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
#include "m0store.h"

#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc != 0)        \
		return __rc; })

/* The REDIS context exists in the TLS, for MT-Safety */
__thread redisContext *rediscontext = NULL;

static int build_m0store_id(kvsns_ino_t         object,
			    struct m0_uint128  *id)
{
	char k[KLEN];
	redisReply *reply;

	snprintf(k, KLEN, "%llu.data", object);
	reply = NULL;
	reply = redisCommand(rediscontext, "GET %s", k);
	if (!reply)
		return -1;

	if (reply->len == 0)
		return -ENOENT;

	*id = M0_CLOVIS_ID_APP;
	id->u_lo = atoi(reply->str);

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

int extstore_create(kvsns_ino_t object, struct stat *stat)
{
	char k[KLEN];
	char v[VLEN];
	redisReply *reply;
	int rc;
	size_t size;
	struct m0_uint128 id;

	snprintf(k, KLEN, "%llu.data", object);
	id = M0_CLOVIS_ID_APP;
	id.u_lo += (unsigned long long)object;
	snprintf(v, VLEN, "%llu", (unsigned long long)id.u_lo);
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	snprintf(k, KLEN, "%llu.data_attr", object);
	size = sizeof(struct stat);
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %b", k, stat, size);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	snprintf(k, KLEN, "%llu.data_ext", object);
	snprintf(v, VLEN, " ");
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;

	freeReplyObject(reply);
	rc = m0store_create_object(id);
	if (rc != 0)
		return rc;

	return 0;
}

int extstore_attach(kvsns_ino_t *ino, char *objid, int objid_len,
		    struct stat *stat)
{
	char k[KLEN];
	char v[VLEN];
	redisReply *reply;
	int rc;
	size_t size;
	struct m0_uint128 id;

	if (objid_len != sizeof(unsigned long long))
		return -EINVAL;


	snprintf(k, KLEN, "%llu.data", *ino);
	id = M0_CLOVIS_ID_APP;
	id.u_lo = atoi(objid);

	snprintf(v, VLEN, "%llu", (unsigned long long)id.u_lo);
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	snprintf(k, KLEN, "%llu.data_attr", *ino);
	size = sizeof(struct stat);
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %b", k, stat, size);
	if (!reply)
		return -1;
	freeReplyObject(reply);

	snprintf(k, KLEN, "%llu.data_ext", *ino);
	snprintf(v, VLEN, " ");
	reply = NULL;
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;

	freeReplyObject(reply);
	rc = m0store_create_object(id);

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
				"Connection error: %s\n",
				rediscontext->errstr);
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

	/* Init m0store */
	return m0store_init();
}

int extstore_del(kvsns_ino_t *ino)
{
	char k[KLEN];
	struct m0_uint128 id;
	int rc;
	redisReply *reply;

	rc = build_m0store_id(*ino, &id);
	if (rc) {
		if (rc == -ENOENT) /* No data created */
			return 0;

		return rc;
	}

	rc = m0store_delete_object(id);
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
	ssize_t read_bytes;
	struct stat storestat;
	struct m0_uint128 id;

	RC_WRAP(build_m0store_id, *ino, &id);

	read_bytes = m0store_pread(id, offset, buffer_size,
				   M0STORE_BLK_SIZE, buffer);
	if (read_bytes < 0)
		return -1;

	RC_WRAP(get_stat, ino, &storestat);
	RC_WRAP(update_stat, &storestat, UP_ST_READ, 0);
	RC_WRAP(set_stat, ino, &storestat);

	return read_bytes;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	ssize_t written_bytes;
	struct m0_uint128 id;
	struct stat objstat;

	RC_WRAP(build_m0store_id, *ino, &id);

	written_bytes = m0store_pwrite(id, offset, buffer_size,
				       M0STORE_BLK_SIZE, buffer);
	if (written_bytes < 0)
		return -1;

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
	struct stat objstat;

	if (!ino || !stat)
		return -EINVAL;

	RC_WRAP(get_stat, ino, &objstat);

	stat->st_size = filesize;
	objstat.st_size = filesize;

	RC_WRAP(update_stat, &objstat, UP_ST_TRUNCATE, filesize);

	stat->st_ctim = objstat.st_ctim;
	stat->st_mtim = objstat.st_mtim;

	RC_WRAP(set_stat, ino, &objstat);

	return 0;
}

