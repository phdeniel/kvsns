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

/* kvsns_internal.c
 * KVSNS: set of internal functions
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include "kvsns_internal.h"

extern struct kvsal_ops kvsal;

int kvsns_next_inode(kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -EINVAL;

	rc = kvsal.incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str)
{
	char *token;
	char *rest = str;
	int maxsize = 0;
	int pos = 0;

	if (!inolist || !str || !size)
		return -EINVAL;

	maxsize = *size;

	while ((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%llu", &inolist[pos++]);

		if (pos == maxsize)
			break;
	}

	*size = pos;

	return 0;
}

int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str)
{
	int i;
	char tmp[VLEN];

	if (!inolist || !str)
		return -EINVAL;

	strcpy(str, "");

	for (i = 0; i < size ; i++)
		if (inolist[i] != 0LL) {
			snprintf(tmp, VLEN, "%llu|", inolist[i]);
			strcat(str, tmp);
		}

	return 0;
}

int kvsns_update_stat(kvsns_ino_t *ino, int flags)
{
	char k[KLEN];
	struct stat stat;

	if (!ino)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.stat", *ino);
	RC_WRAP(kvsal.get_stat, k, &stat);
	RC_WRAP(kvsns_amend_stat, &stat, flags);
	RC_WRAP(kvsal.set_stat, k, &stat);

	return 0;
}

int kvsns_amend_stat(struct stat *stat, int flags)
{
	struct timeval t;

	if (!stat)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	if (flags & STAT_ATIME_SET) {
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_MTIME_SET) {
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_CTIME_SET) {
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_INCR_LINK)
		stat->st_nlink += 1;

	if (flags & STAT_DECR_LINK) {
		if (stat->st_nlink == 1)
			return -EINVAL;

		stat->st_nlink -= 1;
	}
	return 0;
}

int kvsns_get_objectid(kvsns_ino_t *ino,
		       extstore_id_t *eid)
{
	char k[KLEN];

	if (!eid)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.objid", *ino);

	RC_WRAP(kvsal.get_char, k, eid->data);

	eid->len = strnlen(eid->data, VLEN);

	return 0;
}

int kvsns_create_entry(kvsns_cred_t *cred, kvsns_ino_t *parent,
		       char *name, char *lnk, mode_t mode,
		       kvsns_ino_t *new_entry, enum kvsns_type type,
		       extstore_id_t *eid, 
		       int (*nof)(extstore_id_t *, unsigned int, char *))
{
	int rc;
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;
	struct stat parent_stat;
	struct timeval t;

	if (!cred || !parent || !name || !new_entry)
		return -EINVAL;

	if ((type == KVSNS_SYMLINK) && (lnk == NULL))
		return -EINVAL;

	rc = kvsns_lookup(cred, parent, name, new_entry);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP(kvsns_next_inode, new_entry);
	RC_WRAP(kvsns_get_stat, parent, &parent_stat);

	RC_WRAP(kvsal.begin_transaction);

	snprintf(k, KLEN, "%llu.dentries.%s",
		 *parent, name);
	snprintf(v, VLEN, "%llu", *new_entry);

	RC_WRAP_LABEL(rc, aborted, kvsal.set_char, k, v);

	snprintf(k, KLEN, "%llu.parentdir", *new_entry);
	snprintf(v, VLEN, "%llu|", *parent);

	RC_WRAP_LABEL(rc, aborted, kvsal.set_char, k, v);

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_uid = cred->uid; 
	bufstat.st_gid = cred->gid;
	bufstat.st_ino = *new_entry;

	if (gettimeofday(&t, NULL) != 0)
		return -1;

	bufstat.st_atim.tv_sec = t.tv_sec;
	bufstat.st_atim.tv_nsec = 1000 * t.tv_usec;

	bufstat.st_mtim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_mtim.tv_nsec = bufstat.st_atim.tv_nsec;

	bufstat.st_ctim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_ctim.tv_nsec = bufstat.st_atim.tv_nsec;

	switch (type) {
	case KVSNS_DIR:
		bufstat.st_mode = S_IFDIR|mode;
		bufstat.st_nlink = 2;
		break;

	case KVSNS_FILE:
		bufstat.st_mode = S_IFREG|mode;
		bufstat.st_nlink = 1;
		break;

	case KVSNS_SYMLINK:
		bufstat.st_mode = S_IFLNK|mode;
		bufstat.st_nlink = 1;
		break;

	default:
		return -EINVAL;
	}
	snprintf(k, KLEN, "%llu.stat", *new_entry);
	RC_WRAP_LABEL(rc, aborted, kvsal.set_stat, k, &bufstat);

	if (type == KVSNS_SYMLINK) {
		snprintf(k, KLEN, "%llu.link", *new_entry);
		RC_WRAP_LABEL(rc, aborted, kvsal.set_char, k, lnk);
	}

	if (type == KVSNS_FILE) {
		char seed[2*MAXNAMLEN];
		unsigned int seedlen;

		if (!eid) {
			rc = -EINVAL;
			goto aborted;
		}

		if (eid->len == 0) { /* Is eid defined */
			/* eid is provided is file is attached
 			 * otherwise we should create it 
 			 */ 
	
			if (!nof) {
				rc = -EINVAL;
				goto aborted;
			}

			seedlen = snprintf(seed, 2*MAXNAMLEN,
					   "inum=%llu,name=%s",
					   (unsigned long long)*new_entry,
					   name);			
			rc = nof(eid, seedlen, seed);
		}

		/* set the reference in the KVS */
		snprintf(k, KLEN, "%llu.objid", *new_entry);
		RC_WRAP(kvsal.set_char, k, eid->data);
	}
	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, parent, &parent_stat);

	RC_WRAP(kvsal.end_transaction);
	return 0;

aborted:
	kvsal.discard_transaction();
	return rc;
}


/* Access routines */
static int kvsns_access_check(kvsns_cred_t *cred, struct stat *stat, int flags)
{
	int check = 0;

	if (!stat || !cred)
		return -EINVAL;

	/* Root's superpowers */
	if (cred->uid == KVSNS_ROOT_UID)
		return 0;

	if (cred->uid == stat->st_uid) {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_OWNER_READ;

		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_OWNER_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_OWNER_EXEC;
	} else if (cred->gid == stat->st_gid) {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_GROUP_READ;

		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_GROUP_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_GROUP_EXEC;
	} else {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_OTHER_READ;

		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_OTHER_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_OTHER_EXEC;
	}

	if ((check & stat->st_mode) != check)
		return -EPERM;
	else
		return 0;

	/* Should not be reached */
	return -EPERM;
}

int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags)
{
	struct stat stat;

	if (!cred || !ino)
		return -EINVAL;

	RC_WRAP(kvsns_getattr, cred, ino, &stat);

	return kvsns_access_check(cred, &stat, flags);
}

int kvsns_get_stat(kvsns_ino_t *ino, struct stat *bufstat)
{
	char k[KLEN];

	if (!ino || !bufstat)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvsal.get_stat(k, bufstat);
}

int kvsns_set_stat(kvsns_ino_t *ino, struct stat *bufstat)
{
	char k[KLEN];

	if (!ino || !bufstat)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvsal.set_stat(k, bufstat);
}

int kvsns_lookup_path(kvsns_cred_t *cred, kvsns_ino_t *parent, char *path,
		       kvsns_ino_t *ino)
{
	char *saveptr;
	char *str;
	char *token;
	kvsns_ino_t iter_ino = 0LL;
	kvsns_ino_t *iter = NULL;
	int j = 0;
	int rc = 0;

	memcpy(&iter_ino, parent, sizeof(kvsns_ino_t));
	iter = &iter_ino;
	for (j = 1, str = path; ; j++, str = NULL) {
		memcpy(parent, ino, sizeof(kvsns_ino_t));
		token = strtok_r(str, "/", &saveptr);
		if (token == NULL)
			break;

		rc = kvsns_lookup(cred, iter, token, ino);
		if (rc != 0) {
			if (rc == -ENOENT)
				break;
			else
				return rc;
		}

		iter = ino;
	}

	if (token != NULL) /* If non-existing file should be created */
		strcpy(path, token);

	return rc;
}

