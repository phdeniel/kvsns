#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <time.h>
#include "kvsns.h"

static redisContext *rediscontext;

int kvsal_init(void)
{
	unsigned int j;
	redisReply *reply;
	const char *hostname = "127.0.0.1";
	int port = 6379; /* REDIS default */

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	rediscontext = redisConnectWithTimeout(hostname, port, timeout);
	if (rediscontext == NULL || rediscontext->err) {
		if (rediscontext) {
			printf("Connection error: %s\n", rediscontext->errstr);
			redisFree(rediscontext);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
		exit(1);
	}	

	/* PING server */
	reply = redisCommand(rediscontext,"PING");
	if (!reply)
		return -1;

	printf("PING: %s\n", reply->str);
	freeReplyObject(reply);

	return 0;
}

int kvsal_begin_transaction()
{
	redisReply *reply;

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

int kvsal_end_transaction()
{
	redisReply *reply;
	int i;

	reply = redisCommand(rediscontext, "EXEC");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_ARRAY) {
		freeReplyObject(reply);
		return -1;
	}

	for (i=0; i < reply->elements ; i++)
		if (strncmp(reply->element[i]->str, "OK",
			    reply->element[i]->len)) {
			freeReplyObject(reply);
			return -1;
		}

	freeReplyObject(reply);
	return 0;
}

int kvsal_discard_transaction()
{
	redisReply *reply;

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

	/* Set a key */
	reply = redisCommand(rediscontext,"EXISTS %s", k);
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_INTEGER)
		return -1;

	if( reply->integer == 0)
		return -ENOENT;

	freeReplyObject(reply);

	return 0;
}

int kvsal_set_char(char *k, char *v)
{
	redisReply *reply;

	if (!k || !v)
		return -EINVAL;

	/* Set a key */
	reply = redisCommand(rediscontext,"SET %s %s", k, v);
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

	/* Try a GET and two INCR */
	reply = NULL;
	reply = redisCommand(rediscontext,"GET %s", k);
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

	/* Set a key */
	reply = redisCommand(rediscontext,"SET %s %b", k, buf, size);
	if (!reply)
		return -1;

	return 0;
}

int kvsal_get_stat(char *k, struct stat *buf)
{
	redisReply *reply;
	char v[VLEN];
	int rc;

	if (!k || !buf)
		return -EINVAL;

	reply = redisCommand(rediscontext, "GET %s", k);
	if ( !reply )
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

	/* Set a key */
	reply = redisCommand(rediscontext,"SET %s %b", k, buf, size);
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

	reply = redisCommand(rediscontext, "GET %s", k);
	if ( !reply )
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

	reply = redisCommand(rediscontext,"INCR %s", k);
	if (!reply)
		return -1;

    	printf("INCR counter: %lld\n", reply->integer);
	*v = (unsigned long long)reply->integer;

	return 0;
}

int kvsal_del(char *k)
{
	redisReply *reply;

	if (!k)
		return -EINVAL;

	/* Try a GET and two INCR */
	reply = redisCommand(rediscontext,"DEL %s", k);
	if (!reply)
		return -1;
	freeReplyObject(reply);
	return 0;
}

int kvsal_get_list(char *pattern, int start, int *size, kvsal_item_t *items)
{
	redisReply *reply;
	int rc;
	int i;

	if (!pattern || !size || !items)
		return -EINVAL;

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

	reply = redisCommand(rediscontext, "KEYS %s", pattern);
	if (!reply)
		return -1;
	if (reply->type != REDIS_REPLY_ARRAY)
		return -1;
	rc = reply->elements;

	freeReplyObject(reply);
	return rc;
}
	

