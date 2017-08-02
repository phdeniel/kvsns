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

/* kvsns_xattr.c
 * KVSNS: implementation of xattr feature
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include "kvsns_internal.h"

int kvsns_setxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t size, int flags)
{
	int rc;
	char k[KLEN];

	if (!cred || !ino || !name || !value)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.xattr.%s", *ino, name);
	if (flags == XATTR_CREATE) {
		rc = kvsal_get_char(k, value);
		if (rc == 0)
			return -EEXIST;
	}

	return kvsal_set_binary(k, (char *)value, size);
}

int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t *size)
{
	char k[KLEN];

	if (!cred || !ino || !name || !value)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.xattr.%s", *ino, name);
	RC_WRAP(kvsal_get_binary, k, value, size);

	return 0;
}

int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset,
		  kvsns_xattr_t *list, int *size)
{
	int rc;
	char pattern[KLEN];
	kvsal_item_t *items;
	int i;
	kvsal_list_t l;

	if (!cred || !ino || !list || !size)
		return -EINVAL;

	snprintf(pattern, KLEN, "%llu.xattr.*", *ino);
	items = (kvsal_item_t *)malloc(*size*sizeof(kvsal_item_t));
	if (items == NULL)
		return -ENOMEM;

	rc = kvsal_fetch_list(pattern, &l);
	RC_WRAP_LABEL(rc, errout, kvsal_fetch_list, pattern, &l);

	RC_WRAP_LABEL(rc, errout,  kvsal_get_list, &l, offset, size, items);

	RC_WRAP_LABEL(rc, errout, kvsal_dispose_list, &l);

	for (i = 0; i < *size ; i++)
		strncpy(list[i].name, items[i].str, MAXNAMLEN);

	free(items);

	return 0;

errout:
	if (items)
		free(items);

	return rc;
}

int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name)
{
	char k[KLEN];

	snprintf(k, KLEN, "%llu.xattr.%s", *ino, name);
	RC_WRAP(kvsal_del, k);

	return 0;
}

int kvsns_remove_all_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino)
{
	int rc;
	char pattern[KLEN];
	kvsal_item_t items[KVSAL_ARRAY_SIZE];
	int i;
	int size;
	kvsal_list_t list;

	if (!cred || !ino)
		return -EINVAL;

	snprintf(pattern, KLEN, "%llu.xattr.*", *ino);

	rc = kvsal_fetch_list(pattern, &list);
	if (rc < 0)
		return rc;

	do {
		size = KVSAL_ARRAY_SIZE;
		rc = kvsal_get_list(&list, 0, &size, items);
		if (rc < 0)
			return rc;

		for (i = 0; i < size ; i++)
			RC_WRAP(kvsal_del, items[i].str);

	} while (size > 0);

	rc = kvsal_dispose_list(&list);
	if (rc < 0)
		return rc;

	return 0;
}
