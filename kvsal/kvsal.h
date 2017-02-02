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

/* kvsns_test.c
 * KVSNS: simple test
 */

#ifndef _KVSAL_H
#define _KVSAL_H

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/xattr.h>

#define KVSNS_ROOT_INODE 2LL
#define KVSNS_ARRAY_SIZE 100
#define KVSNS_ROOT_UID 0

#define KVSNS_STORE "KVSNS_STORE"
#define KVSNS_SERVER "KVSNS_SERVER"
#define KLEN 256
#define VLEN 256

typedef struct kvsal_item {
	int offset;
	char str[KLEN];
} kvsal_item_t;

typedef struct kvsal_list {
	char pattern[KLEN];
	kvsal_item_t *content;
	size_t size;
} kvsal_list_t;

int kvsal_init(void);
int kvsal_fini(void);
int kvsal_begin_transaction(void);
int kvsal_end_transaction(void);
int kvsal_discard_transaction(void);
int kvsal_exists(char *k);
int kvsal_set_char(char *k, char *v);
int kvsal_get_char(char *k, char *v);
int kvsal_set_binary(char *k, char *buf, size_t size);
int kvsal_get_binary(char *k, char *buf, size_t *size);
int kvsal_set_stat(char *k, struct stat *buf);
int kvsal_get_stat(char *k, struct stat *buf);
int kvsal_get_list_size(char *pattern);
int kvsal_del(char *k);
int kvsal_incr_counter(char *k, unsigned long long *v);

int kvsal_get_list(char *pattern, int start, int *end, kvsal_item_t *items);
int kvsal_get_list2(kvsal_list_t *list, int start, int *end, kvsal_item_t *items);
int kvsal_fetch_list(char *pattern, kvsal_list_t *list);
int kvsal_dispose_list(kvsal_list_t *list);

#endif
