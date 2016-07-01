#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "kvsns.h"
#include "kvshl/kvshl.h"

int kvsns_init(void)
{
	int rc;

	rc = kvshl_init();
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_next_inode(kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -EINVAL;

	rc = kvshl_incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_mkdir(kvsns_ino_t *parent, char *name,
		kvsns_ino_t *newdir)
{
	int rc;
	char k[KLEN];
	char v[KLEN];

	if (!parent || !name || !newdir)
		return -EINVAL;

	rc = kvsns_lookup(parent, name, newdir);
	if (rc == 0)
		return -EEXIST;

	rc = kvsns_next_inode(newdir);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);
	snprintf(v, VLEN, "%llu", *newdir);
	
	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_lookup(kvsns_ino_t *parent, char *name,
		kvsns_ino_t *ino)
{
	int rc;
	char k[KLEN];
	char v[VLEN];

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);

	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	sscanf(v, "%llu", ino);

	return 0;
}

