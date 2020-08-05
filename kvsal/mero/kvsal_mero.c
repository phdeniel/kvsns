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

/* kvsal_mero.c
 * KVS Abstraction Layer: interface for MERO 
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "../../mero/m0common.h"
#include <kvsns/kvsal.h>


/* The REDIS context exists in the TLS, for MT-Safety */

int kvsal_init(struct collection_item *cfg_items)
{
	return m0init(cfg_items);
}

int kvsal_fini(void)
{
	m0fini();

	return 0;
}

int kvsal_begin_transaction(void)
{
	return 0;
}

int kvsal_end_transaction(void)
{
	return 0;
}

int kvsal_discard_transaction(void)
{
	return 0;
}

int kvsal_exists(char *k)
{
	size_t klen;
	size_t vlen = VLEN;
	char myval[VLEN];

	klen = strnlen(k, KLEN)+1;
	return m0kvs_get(k, klen, myval, &vlen);
}

int kvsal_set_char(char *k, char *v)
{
	size_t klen;
	size_t vlen;

	klen = strnlen(k, KLEN)+1;
	vlen = strnlen(v, VLEN)+1;
	return m0kvs_set(k, klen, v, vlen);
}

int kvsal_get_char(char *k, char *v)
{
	size_t klen;
	size_t vlen = VLEN;

	klen = strnlen(k, KLEN)+1;
	return m0kvs_get(k, klen, v, &vlen);
}

int kvsal_set_stat(char *k, struct stat *buf)
{
	size_t klen;

	klen = strnlen(k, KLEN)+1;
	return m0kvs_set(k, klen,
			  (char *)buf, sizeof(struct stat));
}

int kvsal_get_stat(char *k, struct stat *buf)
{
	size_t klen;
	size_t vlen = sizeof(struct stat);

	klen = strnlen(k, KLEN)+1;
	return m0kvs_get(k, klen,
			  (char *)buf, &vlen);
}

int kvsal_set_binary(char *k, char *buf, size_t size)
{
	size_t klen;

	klen = strnlen(k, KLEN)+1;
	return m0kvs_set(k, klen,
			  buf, size);
}

int kvsal_get_binary(char *k, char *buf, size_t *size)
{
	size_t klen;

	klen = strnlen(k, KLEN)+1;
	return m0kvs_get(k, klen,
			  buf, size);
}

int kvsal_incr_counter(char *k, unsigned long long *v)
{
	int rc;
	char buf[VLEN];
	size_t vlen = VLEN;
	size_t klen;

	klen = strnlen(k, KLEN) + 1;

	rc = m0kvs_get(k, klen, buf, &vlen);
	if (rc != 0)
		return rc;

	sscanf(buf, "%llu", v);
	*v += 1; 
	snprintf(buf, VLEN, "%llu", *v);
	vlen = strnlen(buf, VLEN)+1;

	rc = m0kvs_set(k, klen, buf, vlen);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsal_del(char *k)
{
	size_t klen;

	klen = strnlen(k, KLEN)+1;
	return m0kvs_del(k, klen);
}

bool get_list_cb_size(char *k, void *arg)
{
	int size;

	memcpy((char *)&size, (char *)arg, sizeof(int));
	size += 1;
	memcpy((char *)arg, (char *)&size, sizeof(int));

	return true;
}

int kvsal_init_list(kvsal_list_t *list)
{
	if (!list)
		return -EINVAL;

	list->size = 0;
	list->content = NULL;

	return 0;
}


int kvsal_get_list_size(char *pattern)
{
	char initk[KLEN];
	int size = 0;
	int rc;

	strncpy(initk, pattern, KLEN);
	initk[strnlen(pattern, KLEN)-1] = '\0';

	rc = m0_pattern_kvs(initk, pattern,
			    get_list_cb_size, &size);
	if (rc < 0)
		return rc;

	return size;
}

bool populate_list(char *k, void *arg)
{
	kvsal_list_t *list;
	kvsal_item_t *item;
	kvsal_item_t *content;

	list = (kvsal_list_t *)arg;

	if (!list)
		return false;

	list->size +=1;
	content = list->content;

	list->content = realloc(content, list->size*sizeof(kvsal_item_t));
	if (list->content == NULL)
		return false;

	item = &list->content[list->size - 1];

	strncpy(item->str, k, KLEN);
	item->offset = list->size -1;

	return true;
}

int kvsal_fetch_list(char *pattern, kvsal_list_t *list)
{
	char initk[KLEN];
	int rc = 0;

        if (!pattern || !list)
                return -EINVAL;


	strncpy(initk, pattern, KLEN);
	initk[strnlen(pattern, KLEN)-1] = '\0';

	rc =  m0_pattern_kvs(initk, pattern,
			     populate_list, list);

	if (rc == -ENOENT)
		rc = 0; /* Empty list */

	return rc;
}

int kvsal_dispose_list(kvsal_list_t *list)
{
        if (!list)
                return -EINVAL;

        return 0;
}

/** @todo: too many strncpy and mallocs, this should be optimized */
int kvsal_get_list(kvsal_list_t *list, int start, int *size,
                    kvsal_item_t *items)
{
	int i;

	if (list->size < (start + *size))
		*size = list->size - start;


	for (i = start; i < start + *size ; i++) {
		items[i-start].offset = i;
		strncpy(items[i-start].str,
			list->content[i].str, KLEN);
	}

	return 0;
}

/* Since the implementation should preftech the list in memory,
 * this call can become pretty expensive */
int kvsal_get_list_pattern(char *pattern, int start, int *size,
			   kvsal_item_t *items)
{
	char initk[KLEN];
	kvsal_list_t list;
	int rc;

	strncpy(initk, pattern, KLEN);
	initk[strnlen(pattern, KLEN)-1] = '\0';

	rc = kvsal_fetch_list(pattern, &list);
	if (rc < 0 )
		return rc;

	rc = kvsal_get_list(&list, start, size, items);
	if (rc < 0) {
		kvsal_dispose_list(&list); /* Try to clean up */
		return rc;
	}

	rc = kvsal_dispose_list(&list);
	if (rc < 0 )
		return rc;

	return 0;
}
