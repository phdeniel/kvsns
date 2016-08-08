#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kvsns.h"
#include "kvsns_internal.h"

static int kvsns_str2ownerlist(kvsns_open_owner_t *ownerlist, int *size,
			        char *str)
{
	char *token;
	char *rest = str;
	int maxsize = *size;
	int pos = 0;

	if (!ownerlist || !str || !size)
		return -EINVAL;

	while((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%llu.%llu",
		       &ownerlist[pos].pid, 
		       &ownerlist[pos++].thrid);

		if (pos == maxsize)
			break;
	}

	*size = pos;

	return 0;
}

static int kvsns_ownerlist2str(kvsns_open_owner_t *ownerlist, int size,
			       char *str)
{
	int i;
	char tmp[VLEN];

	if (!ownerlist || !str)
		return -EINVAL;

	strcpy(str, "");

	for (i=0; i < size ; i++)
		if (ownerlist[i].pid != 0LL) {
			snprintf(tmp, VLEN, "%llu.%llu|", 
				 ownerlist[i].pid, ownerlist[i].thrid);
			strcat(str, tmp);
		}
	
	return 0;	
}


int kvsns_creat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newfile)
{
	int rc;

	RC_WRAP(rc, kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);
	return kvsns_create_entry(cred, parent, name,
				  mode, newfile, KVSNS_FILE);
}

int kvsns_open(kvsns_cred_t *cred, kvsns_ino_t *ino, 
	       int flags, mode_t mode, kvsns_file_open_t *fd)
{
	kvsns_open_owner_t me;
	kvsns_open_owner_t owners[KVSNS_ARRAY_SIZE];
	int size = KVSNS_ARRAY_SIZE;
	char k[KLEN];
	char v[VLEN];
	int rc;

	if (!cred || !ino || !fd)
		return -EINVAL;

	/** @todo Put here the access control base on flags and mode values */
	me.pid = getpid();
	me.thrid = pthread_self();

	/* Manage the list of open owners */
	snprintf(k, KLEN, "%llu.openowner", *ino);
	RC_WRAP(rc, kvsal_get_char, k, v);

	RC_WRAP(rc, kvsns_str2ownerlist, owners, &size, v);
	if (size == KVSNS_ARRAY_SIZE)
		return -EMLINK; /* Too many open files */
	owners[size].pid = me.pid;
	owners[size].thrid = me.thrid;
	size += 1;
	RC_WRAP(rc, kvsns_ownerlist2str, owners, size, v);

	RC_WRAP(rc, kvsal_get_char, k, v);

	/** todo Do not forget store stuffs */
	fd->store_handle = 0;
	/* In particular create a key per opened fd */
	
	return 0;
}

int kvsns_openat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 int flags, mode_t mode, kvsns_file_open_t *fd)
{
	kvsns_ino_t ino;
	int rc;

	if (!cred || !parent || !name || !fd)
		return -EINVAL;

	RC_WRAP(rc, kvsns_lookup, cred, parent, name, &ino);

	return kvsns_open(cred, &ino, flags, mode, fd);
}

int kvsns_close(kvsns_file_open_t *fd)
{
	kvsns_open_owner_t owners[KVSNS_ARRAY_SIZE];
	int size = KVSNS_ARRAY_SIZE;
	kvsns_open_owner_t me;
	char k[KLEN];
	char v[VLEN];
	int i;
	int rc;

	if (!fd)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.openowner", fd->ino);
	RC_WRAP(rc, kvsal_get_char, k, v);

	RC_WRAP(rc, kvsns_str2ownerlist, owners, &size, v);
	for (i=0; i < size ; i++)
		if (owners[i].pid == fd->owner.pid &&
		    owners[i].thrid == fd->owner.thrid) {
			owners[i].pid = 0; /* remove it from list */ 
			break;
		}
	RC_WRAP(rc, kvsns_ownerlist2str, owners, size, v);

	RC_WRAP(rc, kvsal_get_char, k, v);

	/** todo Do not forget store stuffs */

	return 0;
}

ssize_t kvsns_pwrite(kvsns_cred_t *cred, kvsns_file_open_t *fd, 
		     void *buf, size_t count, off_t offset)
{
	return 0;
}

ssize_t kvsns_pread(kvsns_cred_t *cred, kvsns_file_open_t *fd, 
		    void *buf, size_t count, off_t offset)
{
	return 0;
}

