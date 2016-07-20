#ifndef _KVSAL_H
#define  _KVSAL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define KLEN 256
#define VLEN 256

typedef struct kvsal_item__ {
	int offset;
	char str[KLEN];
} kvsal_item_t;

int kvsal_init();
int kvsal_begin_transaction();
int kvsal_end_transaction();
int kvsal_set_char(char *k, char *v);
int kvsal_get_char(char *k, char *v);
int kvsal_set_stat(char *k, struct stat *buf);
int kvsal_get_stat(char *k, struct stat *buf);
int kvsal_get_list_size(char *pattern);
int kvsal_get_list(char *pattern, int start, int *end, kvsal_item_t *items);
int kvsal_del(char *k);
int kvsal_incr_counter(char *k, unsigned long long *v);
#endif

