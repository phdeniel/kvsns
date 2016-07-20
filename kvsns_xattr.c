#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kvsns.h"
#include "kvsns_internal.h"
#include "kvsal/kvsal.h"

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

	return kvsal_set_char(k, (char *)value);
}

int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t size)
{
	int rc;
	char k[KLEN];
	char v[VLEN];

	if (!cred || !ino || !name || !value)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.xattr.%s", *ino, name);
	rc = kvsal_get_char(k, v);
	if (rc != 0)
		return rc;

	strncpy(value, v, size);
	return 0; 
}

int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset, 
		  kvsns_xattr_t *list, int *size)
{
	int rc;
	char pattern[KLEN];
	char v[VLEN];
	kvsal_item_t *items;
	int i; 
	kvsns_ino_t tmpino;

	if (!cred || !ino || !list || !size)
		return -EINVAL;

	items = (kvsal_item_t *)malloc(*size*sizeof(kvsal_item_t));
	if (items == NULL)
		return -ENOMEM;

	kvsal_begin_transaction();

	rc = kvsal_get_list(pattern, offset, size, items);
	if (rc < 0)
		return rc;

	for (i=0; i < *size ; i++)
		strncpy(list[i].name, items[i].str, MAXNAMLEN);


	return 0;
}

int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name)
{
	int rc;
	char k[KLEN];

	snprintf(k, KLEN, "%llu.xattr.%s", *ino, name);
	return rc = kvsal_del(k);
}

