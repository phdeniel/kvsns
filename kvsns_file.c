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

/* kvsns_file.c
 * KVSNS: functions related to file management
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kvsal/kvsal.h"
#include "kvsns.h"
#include "kvsns_internal.h"
#include "extstore.h"

extern int kvsns_debug;

static int kvsns_str2ownerlist(kvsns_open_owner_t *ownerlist, int *size,
			        char *str)
{
	char *token;
	char *rest = str;
	int maxsize = *size;
	int pos = 0;

	if (!ownerlist || !str || !size)
		return -EINVAL;

	pos = 0;
	while((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%llu.%llu",
		       &ownerlist[pos].pid,
		       &ownerlist[pos].thrid);
		pos += 1;
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

	for (i = 0; i < size ; i++)
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
	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);
	RC_WRAP(kvsns_create_entry, cred, parent, name, NULL,
				  mode, newfile, KVSNS_FILE);
	RC_WRAP(extstore_create, *newfile);

	return 0;
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

	if (kvsns_debug)
		fprintf(stderr, "kvsns_open\n");

	if (!cred || !ino || !fd)
		return -EINVAL;

	/** @todo Put here the access control base on flags and mode values */
	me.pid = getpid();
	me.thrid = pthread_self();

	/* Manage the list of open owners */
	snprintf(k, KLEN, "%llu.openowner", *ino);
	rc = kvsal_get_char(k, v);
	if (rc == 0) {
		RC_WRAP(kvsns_str2ownerlist, owners, &size, v);
		if (size == KVSNS_ARRAY_SIZE)
			return -EMLINK; /* Too many open files */
		owners[size].pid = me.pid;
		owners[size].thrid = me.thrid;
		size += 1;
		RC_WRAP(kvsns_ownerlist2str, owners, size, v);
	} else if (rc == -ENOENT) {
		/* Create the key => 1st fd created */
		snprintf(v, VLEN, "%llu.%llu|", me.pid, me.thrid);
	} else
		return rc;

	RC_WRAP(kvsal_set_char, k, v);

	/** @todo Do not forget store stuffs */
	fd->ino = *ino;
	fd->owner.pid = me.pid;
	fd->owner.thrid = me.thrid;
	fd->flags = flags;

	/* In particular create a key per opened fd */

	return 0;
}

int kvsns_openat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 int flags, mode_t mode, kvsns_file_open_t *fd)
{
	kvsns_ino_t ino;

	if (kvsns_debug)
		fprintf(stderr, "kvsns_openat\n");

	if (!cred || !parent || !name || !fd)
		return -EINVAL;

	RC_WRAP(kvsns_lookup, cred, parent, name, &ino);

	return kvsns_open(cred, &ino, flags, mode, fd);
}

int kvsns_close(kvsns_file_open_t *fd)
{
	kvsns_open_owner_t owners[KVSNS_ARRAY_SIZE];
	int size = KVSNS_ARRAY_SIZE;
	char k[KLEN];
	char v[VLEN];
	int i;
	int rc;
	bool found = false;
	bool opened_and_deleted;

	if (kvsns_debug)
		fprintf(stderr, "kvsns_close\n");

	if (!fd)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.openowner", fd->ino);
	rc = kvsal_get_char(k, v);
	if (rc != 0) {
		if (rc == -ENOENT)
			return -EBADF; /* File not opened */
		else
			return rc;
	}

	/* Was the file deleted as it was opened ? */
	/* The last close should perform actual data deletion */
	snprintf(k, KLEN, "%llu.opened_and_deleted", fd->ino);
	rc = kvsal_exists(k);
	if ((rc != 0) && (rc != -ENOENT))
		return rc;
	opened_and_deleted = (rc == -ENOENT) ? false : true;

	RC_WRAP(kvsns_str2ownerlist, owners, &size, v);

	RC_WRAP(kvsal_begin_transaction);

	if (size == 1) {
		if (fd->owner.pid == owners[0].pid &&
		    fd->owner.thrid == owners[0].thrid) {
			snprintf(k, KLEN, "%llu.openowner", fd->ino);
			RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

			/* Was the file deleted as it was opened ? */
			if (opened_and_deleted) {
				RC_WRAP_LABEL(rc, aborted,
					      extstore_del, &fd->ino);
				snprintf(k, KLEN, "%llu.opened_and_deleted",
					 fd->ino);
				RC_WRAP_LABEL(rc, aborted,
					      kvsal_del, k);
			}
			RC_WRAP(kvsal_end_transaction);
			return 0;
		} else {
			rc = -EBADF;
			goto aborted;
		}
	} else {
		found = false;
		for (i = 0; i < size ; i++)
			if (owners[i].pid == fd->owner.pid &&
			    owners[i].thrid == fd->owner.thrid) {
				owners[i].pid = 0; /* remove it from list */
				found = true;
				break;
			}
	}


	if (!found) {
		rc = -EBADF;
		goto aborted;
	}

	RC_WRAP_LABEL(rc, aborted, kvsns_ownerlist2str, owners, size, v);

	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	RC_WRAP(kvsal_end_transaction);
	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

ssize_t kvsns_write(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		    void *buf, size_t count, off_t offset)
{
	size_t write_amount;
	bool stable;
	char k[KLEN];
	struct stat stat;
	int statflags = STAT_SIZE_SET|STAT_ATIME_SET|STAT_MTIME_SET|
			STAT_CTIME_SET;

	if (kvsns_debug)
		fprintf(stderr, "kvsns_write\n");

	RC_WRAP(kvsns_getattr, cred, &fd->ino, &stat);

	/** @todo use flags to check correct access */
	write_amount = extstore_write(&fd->ino,
				      offset,
				      count,
				      buf,
				      &stable,
				      &stat);
	if (write_amount < 0)
		return write_amount;

	snprintf(k, KLEN, "%llu.stat", fd->ino);
	RC_WRAP(kvsal_set_stat, k, &stat);

	return write_amount;
}

ssize_t kvsns_read(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		   void *buf, size_t count, off_t offset)
{
	size_t read_amount;
	bool eof;
	struct stat stat;
	char k[KLEN];
	int statflags = STAT_SIZE_SET|STAT_ATIME_SET|STAT_MTIME_SET|
			STAT_CTIME_SET;

	if (kvsns_debug)
		fprintf(stderr, "kvsns_read\n");

	RC_WRAP(kvsns_getattr, cred, &fd->ino, &stat);

	/** @todo use flags to check correct access */
	read_amount = extstore_read(&fd->ino,
				    offset,
				    count,
				    buf,
				    &eof,
				    &stat);
	if (read_amount < 0)
		return read_amount;

	snprintf(k, KLEN, "%llu.stat", fd->ino);
	RC_WRAP(kvsal_set_stat, k, &stat);

	return read_amount;
}

