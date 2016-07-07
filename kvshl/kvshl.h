#ifndef _KVSHL_H
#define  _KVSHL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define KLEN 256
#define VLEN 256

typedef struct kvshl_item__ {
	int offset;
	char str[KLEN];
} kvshl_item_t;

int kvshl_init();
int kvshl_begin_transaction();
int kvshl_end_transaction();
int kvshl_set_char(char *k, char *v);
int kvshl_get_char(char *k, char *v);
int kvshl_set_stat(char *k, struct stat *buf);
int kvshl_get_stat(char *k, struct stat *buf);
int kvshl_get_list_size(char *pattern);
int kvshl_get_list(char *pattern, int start, int *end, kvshl_item_t *items);
int kvshl_del(char *k);
int kvshl_incr_counter(char *k, unsigned long long *v);
#endif

