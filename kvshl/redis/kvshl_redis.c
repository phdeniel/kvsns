#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "../kvshl.h"

static redisContext *rediscontext;

int kvshl_init(void)
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

int kvshl_set_char(char *k, char *v)
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

int kvshl_incr_counter(char *k, unsigned long long *v)
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


int kvshl_get_char(char *k, char *v)
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

int kvshl_del(char *k)
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
