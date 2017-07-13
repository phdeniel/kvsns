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

/* kvsal_redis.c
 * KVS Abstraction Layer: interface for REDIS
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <time.h>
#include <ini_config.h>
#include <kvsns/kvsal.h>

#define RC_WRAP(__function, ...) ({\
        int __rc = __function(__VA_ARGS__);\
        if (__rc != 0)        \
                return __rc; })

/* The REDIS context exists in the TLS, for MT-Safety */
__thread redisContext *rediscontext = NULL;

static struct collection_item *conf = NULL;

int kvsal_init(struct collection_item *cfg_items)
{
	unsigned int j;
	redisReply *reply;
	char *hostname = NULL;
	char hostname_default[] = "127.0.0.1";
	struct timeval timeout = { 1, 500000 }; /* 1.5 seconds */
	int port = 6379; /* REDIS default */
	struct collection_item *item = NULL;

	if (cfg_items == NULL)
		return -EINVAL;

	if (conf == NULL)
		conf = cfg_items;

	/* Get config from ini file */
	item = NULL;
	RC_WRAP(get_config_item, "kvsal_redis", "server", cfg_items, &item);
	if (item == NULL)
		hostname = hostname_default;
	else
		hostname = get_string_config_value(item, NULL);

	item = NULL;
	RC_WRAP(get_config_item, "kvsal_redis", "port", cfg_items, &item);
	if (item != NULL)
		port = (int)get_int_config_value(item, 0, 0, NULL);

	/* Start REDIS */
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

	return 0;
}

static int kvsal_reinit()
{
	return kvsal_init(conf);
}

int kvsal_fini(void)
{
	return 0;
}

int kvsal_begin_transaction(void)
{
	redisReply *reply;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "MULTI");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STATUS) {
		freeReplyObject(reply);
		return -1;
	}

	if (strncmp(reply->str, "OK", reply->len)) {
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;
}

int kvsal_end_transaction(void)
{
	redisReply *reply;
	int i;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "EXEC");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_ARRAY) {
		freeReplyObject(reply);
		return -1;
	}

	for (i = 0; i < reply->elements ; i++)
		if (strncmp(reply->element[i]->str, "OK",
			    reply->element[i]->len)) {
			freeReplyObject(reply);
			return -1;
		}

	freeReplyObject(reply);
	return 0;
}

int kvsal_discard_transaction(void)
{
	redisReply *reply;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "DISCARD");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STATUS) {
		freeReplyObject(reply);
		return -1;
	}

	if (strncmp(reply->str, "OK", reply->len)) {
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);
	return 0;
}

int kvsal_exists(char *k)
{
	redisReply *reply;
	int rc;

	if (!k)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Set a key */
	reply = redisCommand(rediscontext, "EXISTS %s", k);
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_INTEGER)
		return -1;

	if (reply->integer == 0)
		return -ENOENT;

	freeReplyObject(reply);

	return 0;
}

int kvsal_set_char(char *k, char *v)
{
	redisReply *reply;

	if (!k || !v)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %s %s", k, v);
	if (!reply)
		return -1;

	freeReplyObject(reply);

	return 0;
}

int kvsal_get_char(char *k, char *v)
{
	redisReply *reply;

	if (!k || !v)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Try a GET and two INCR */
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

int kvsal_set_stat(char *k, struct stat *buf)
{
	redisReply *reply;

	size_t size = sizeof(struct stat);

	if (!k || !buf)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %s %b", k, buf, size);
	if (!reply)
		return -1;

	freeReplyObject(reply);

	return 0;
}

int kvsal_get_stat(char *k, struct stat *buf)
{
	redisReply *reply;
	char v[VLEN];
	int rc;

	if (!k || !buf)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

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

int kvsal_set_binary(char *k, char *buf, size_t size)
{
	redisReply *reply;

	if (!k || !buf)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %s %b", k, buf, size);
	if (!reply)
		return -1;

	return 0;
}

int kvsal_get_binary(char *k, char *buf, size_t *size)
{
	redisReply *reply;
	char v[VLEN];
	int rc;

	if (!k || !buf || !size)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "GET %s", k);
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STRING)
		return -1;

	if (reply->len > *size)
		return -1;

	memcpy((char *)buf, reply->str, reply->len);
	*size = reply->len;

	freeReplyObject(reply);

	return 0;
}

int kvsal_incr_counter(char *k, unsigned long long *v)
{
	redisReply *reply;

	if (!k || !v)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "INCR %s", k);
	if (!reply)
		return -1;

	*v = (unsigned long long)reply->integer;

	return 0;
}

int kvsal_del(char *k)
{
	redisReply *reply;

	if (!k)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Try a GET and two INCR */
	reply = redisCommand(rediscontext, "DEL %s", k);
	if (!reply)
		return -1;
	freeReplyObject(reply);
	return 0;
}

int kvsal_get_list_pattern(char *pattern, int start, int *size,
			   kvsal_item_t *items)
{
	redisReply *reply;
	int rc;
	int i;

	if (!pattern || !size || !items)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "KEYS %s", pattern);
	if (!reply)
		return -1;
	if (reply->type != REDIS_REPLY_ARRAY)
		return -1;

	if (reply->elements < (start + *size))
		*size = reply->elements - start;

	for (i = start; i < start + *size ; i++) {
		items[i-start].offset = i;
		strcpy(items[i-start].str, reply->element[i]->str);
	}

	freeReplyObject(reply);
	return 0;
}

int kvsal_get_list_size(char *pattern)
{
	redisReply *reply;
	int rc;

	if (!pattern)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "KEYS %s", pattern);
	if (!reply)
		return -1;
	if (reply->type != REDIS_REPLY_ARRAY)
		return -1;
	rc = reply->elements;

	freeReplyObject(reply);
	return rc;
}

int kvsal_init_list(kvsal_list_t *list)
{
	if (!list)
		return -EINVAL;

	list->size = 0;
	list->content = NULL;

	return 0;
}

int kvsal_fetch_list(char *pattern, kvsal_list_t *list)
{
	if (!pattern || !list)
		return -EINVAL;

	/* REDIS manages KVS in RAM. Nothing to do */
	strncpy(list->pattern , pattern, KLEN);

	return 0;
}

int kvsal_dispose_list(kvsal_list_t *list)
{
	if (!list)
		return -EINVAL;

	/* REDIS manages KVS in RAM. Nothing to do */
	return 0;
}

int kvsal_get_list(kvsal_list_t *list, int start, int *end,
		    kvsal_item_t *items)
{
	if (!list)
		return -EINVAL;
	
	return kvsal_get_list_pattern(list->pattern, 
				      start, 
				      end,
				      items);
}
