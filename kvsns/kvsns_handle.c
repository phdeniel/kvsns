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

/* kvsns_handle.c
 * KVSNS: functions to manage handles
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include <kvsns/extstore.h>
#include "kvsns_internal.h"

int kvsns_fsstat(kvsns_fsstat_t *stat)
{
	char k[KLEN];
	int rc;

	if (!stat)
		return -EINVAL;

	snprintf(k, KLEN, "*.stat");
	rc = kvsal_get_list_size(k);
	if (rc < 0)
		return rc;

	stat->nb_inodes = rc;
	return 0;
}

int kvsns_get_root(kvsns_ino_t *ino)
{
	if (!ino)
		return -EINVAL;

	*ino = KVSNS_ROOT_INODE;
	return 0;
}

int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir)
{
	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);

	return kvsns_create_entry(cred, parent, name, NULL,
				  mode, newdir, KVSNS_DIR);
}

int kvsns_symlink(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  char *content, kvsns_ino_t *newlnk)
{
	struct stat parent_stat;

	if (!cred || !parent || !name || !content || !newlnk)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);
	RC_WRAP(kvsns_get_stat, parent, &parent_stat);

	RC_WRAP(kvsns_create_entry, cred, parent, name, content,
		0, newlnk, KVSNS_SYMLINK);

	RC_WRAP(kvsns_update_stat, parent, STAT_MTIME_SET|STAT_CTIME_SET);

	return 0;
}

int kvsns_readlink(kvsns_cred_t *cred, kvsns_ino_t *lnk,
		  char *content, size_t *size)
{
	char k[KLEN];
	char v[KLEN];

	/* No access check, a symlink's content is always readable */
	if (!cred || !lnk || !content || !size)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.link", *lnk);
	RC_WRAP(kvsal_get_char, k, v);

	strncpy(content, v, *size);
	*size = strnlen(v, VLEN);

	RC_WRAP(kvsns_update_stat, lnk, STAT_ATIME_SET);

	return 0;
}

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name)
{
	int rc;
	char k[KLEN];
	kvsns_ino_t ino = 0LL;
	struct stat parent_stat;

	if (!cred || !parent || !name)
		return -EINVAL;

	memset(&parent_stat, 0, sizeof(parent_stat));

	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);

	RC_WRAP(kvsns_lookup, cred, parent, name, &ino);

	RC_WRAP(kvsns_get_stat, parent, &parent_stat);

	snprintf(k, KLEN, "%llu.dentries.*", ino);
	rc = kvsal_get_list_size(k);
	if (rc > 0)
		return -ENOTEMPTY;

	RC_WRAP(kvsal_begin_transaction);

	snprintf(k, KLEN, "%llu.dentries.%s",
		 *parent, name);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	snprintf(k, KLEN, "%llu.parentdir", ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	snprintf(k, KLEN, "%llu.stat", ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, parent, &parent_stat);

	RC_WRAP(kvsal_end_transaction);

	/* Remove all associated xattr */
	RC_WRAP(kvsns_remove_all_xattr, cred, &ino);

	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

int kvsns_opendir(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_dir_t *ddir)
{
	char pattern[KLEN];
	if (!cred || ! dir || !ddir)
		return -EINVAL;
	
	snprintf(pattern, KLEN, "%llu.dentries.*", *dir);

	ddir->ino = *dir;
	return kvsal_fetch_list(pattern , &ddir->list);
}

int kvsns_closedir(kvsns_dir_t *dir)
{
	if (!dir)
		return -EINVAL;

	return kvsal_dispose_list(&dir->list);
}

int kvsns_readdir(kvsns_cred_t *cred, kvsns_dir_t *dir, off_t offset,
		  kvsns_dentry_t *dirent, int *size)
{
	char pattern[KLEN];
	char v[VLEN];
	kvsal_item_t *items;
	int i;
	kvsns_ino_t ino = 0LL;
	int rc;
	unsigned long long lino;

	if (!cred || !dir || !dirent || !size)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, &dir->ino, KVSNS_ACCESS_READ);

	items = (kvsal_item_t *)malloc(*size*sizeof(kvsal_item_t));
	if (items == NULL)
		return -ENOMEM;
	memset(items, 0, *size*sizeof(kvsal_item_t));

	memcpy(&lino, dir, sizeof(lino)); /* violent cast */
	snprintf(pattern, KLEN, "%llu.dentries.*", lino);
	rc = kvsal_get_list(&dir->list, (int)offset, size, items);
	RC_WRAP_LABEL(rc, errout,
		      kvsal_get_list, &dir->list, (int)offset, size, items);

	for (i = 0; i < *size ; i++) {
		sscanf(items[i].str, "%llu.dentries.%[^\n]",
		       &ino, dirent[i].name);

		RC_WRAP_LABEL(rc, errout, kvsal_get_char, items[i].str, v);

		sscanf(v, "%llu", &dirent[i].inode);

		RC_WRAP_LABEL(rc, errout, kvsns_getattr, cred, &dirent[i].inode,
			 &dirent[i].stats);
	}

	RC_WRAP_LABEL(rc, errout, kvsns_update_stat, &dir->ino, STAT_ATIME_SET);

	free(items);
	return 0;

errout:
	if (items)
		free(items);

	return rc;
}

int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		kvsns_ino_t *ino)
{
	char k[KLEN];
	char v[VLEN];

	if (!cred || !parent || !name || !ino)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_READ);

	snprintf(k, KLEN, "%llu.dentries.%s",
		 *parent, name);

	RC_WRAP(kvsal_get_char, k, v);

	sscanf(v, "%llu", ino);

	return 0;
}

int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent)
{
	char k[KLEN];
	char v[VLEN];

	if (!cred || !dir || !parent)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, dir, KVSNS_ACCESS_READ);

	snprintf(k, KLEN, "%llu.parentdir",
		 *dir);

	RC_WRAP(kvsal_get_char, k, v);

	sscanf(v, "%llu|", parent);

	return 0;
}

int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *bufstat)
{
	struct stat data_stat;
	char k[KLEN];
	int rc;

	if (!cred || !ino || !bufstat)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.stat", *ino);
	RC_WRAP(kvsal_get_stat, k, bufstat);

	if (S_ISREG(bufstat->st_mode)) {
		/* for file, information is to be retrieved form extstore */
		rc = extstore_getattr(ino, &data_stat);
		if (rc != 0) {
			if (rc == -ENOENT)
				return 0; /* no associated data */
			else
				return rc;
		}

		/* found associated data and store metadata */
		bufstat->st_size = data_stat.st_size;
		bufstat->st_size = data_stat.st_size;
		bufstat->st_mtime = data_stat.st_mtime;
		bufstat->st_atime = data_stat.st_atime;
	}

	return 0;
}

int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		  struct stat *setstat, int statflag)
{
	char k[KLEN];
	struct stat bufstat;
	struct timeval t;
	mode_t ifmt;

	if (!cred || !ino || !setstat)
		return -EINVAL;

	if (statflag == 0)
		return 0; /* Nothing to do */

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	RC_WRAP(kvsns_access, cred, ino, KVSNS_ACCESS_WRITE);

	snprintf(k, KLEN, "%llu.stat", *ino);
	RC_WRAP(kvsal_get_stat, k, &bufstat);

	/* ctime is to be updated if md are changed */
	bufstat.st_ctim.tv_sec = t.tv_sec;
	bufstat.st_ctim.tv_nsec = 1000 * t.tv_usec;

	if (statflag & STAT_MODE_SET) {
		ifmt = bufstat.st_mode & S_IFMT;
		bufstat.st_mode = setstat->st_mode | ifmt;
	}

	if (statflag & STAT_UID_SET)
		bufstat.st_uid = setstat->st_uid;

	if (statflag & STAT_GID_SET)
		bufstat.st_gid = setstat->st_gid;

	if (statflag & STAT_SIZE_SET)
		RC_WRAP(extstore_truncate, ino, setstat->st_size, true,
			&bufstat);

	if (statflag & STAT_SIZE_ATTACH)
		RC_WRAP(extstore_truncate, ino, setstat->st_size, false,
			&bufstat);

	if (statflag & STAT_ATIME_SET) {
		bufstat.st_atim.tv_sec = setstat->st_atim.tv_sec;
		bufstat.st_atim.tv_nsec = setstat->st_atim.tv_nsec;
	}

	if (statflag & STAT_MTIME_SET) {
		bufstat.st_mtim.tv_sec = setstat->st_mtim.tv_sec;
		bufstat.st_mtim.tv_nsec = setstat->st_mtim.tv_nsec;
	}

	if (statflag & STAT_CTIME_SET) {
		bufstat.st_ctim.tv_sec = setstat->st_ctim.tv_sec;
		bufstat.st_ctim.tv_nsec = setstat->st_ctim.tv_nsec;
	}

	return kvsal_set_stat(k, &bufstat);
}

int kvsns_link(kvsns_cred_t *cred, kvsns_ino_t *ino,
	       kvsns_ino_t *dino, char *dname)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t tmpino = 0LL;
	struct stat dino_stat;
	struct stat ino_stat;

	if (!cred || !ino || !dino || !dname)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, dino, KVSNS_ACCESS_WRITE);

	rc = kvsns_lookup(cred, dino, dname, &tmpino);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP(kvsns_get_stat, dino, &dino_stat);
	RC_WRAP(kvsns_get_stat, ino, &ino_stat);


	snprintf(k, KLEN, "%llu.parentdir", *ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_get_char, k, v);

	snprintf(k, KLEN, "%llu|", *dino);
	strcat(v, k);

	RC_WRAP(kvsal_begin_transaction);

	snprintf(k, KLEN, "%llu.parentdir", *ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	snprintf(k, KLEN, "%llu.dentries.%s",
		 *dino, dname);
	snprintf(v, VLEN, "%llu", *ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &ino_stat,
		      STAT_CTIME_SET|STAT_INCR_LINK);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, ino, &ino_stat);

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &dino_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, dino, &dino_stat);

	RC_WRAP(kvsal_end_transaction);

	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *dir, char *name)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino = 0LL;
	kvsns_ino_t parent[KVSAL_ARRAY_SIZE];
	struct stat ino_stat;
	struct stat dir_stat;
	int size;
	int i;
	bool opened;
	bool deleted;

	opened = false;
	deleted = false;

	if (!cred || !dir || !name)
		return -EINVAL;

	memset(parent, 0, KVSAL_ARRAY_SIZE*sizeof(kvsns_ino_t));
	memset(&ino_stat, 0, sizeof(ino_stat));
	memset(&dir_stat, 0, sizeof(dir_stat));

	RC_WRAP(kvsns_access, cred, dir, KVSNS_ACCESS_WRITE);

	RC_WRAP(kvsns_lookup, cred, dir, name, &ino);

	RC_WRAP(kvsns_get_stat, dir, &dir_stat);
	RC_WRAP(kvsns_get_stat, &ino, &ino_stat);

	snprintf(k, KLEN, "%llu.parentdir", ino);
	RC_WRAP(kvsal_get_char, k, v);

	size = KVSAL_ARRAY_SIZE;
	RC_WRAP(kvsns_str2parentlist, parent, &size, v);

	/* Check if file is opened */
	snprintf(k, KLEN, "%llu.openowner", ino);
	rc = kvsal_exists(k);
	if ((rc != 0) && (rc != -ENOENT))
		return rc;

	opened = (rc == -ENOENT) ? false : true;

	RC_WRAP(kvsal_begin_transaction);

	if (size == 1) {
		/* Last link, try to perform deletion */
		snprintf(k, KLEN, "%llu.parentdir", ino);
		RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

		snprintf(k, KLEN, "%llu.stat", ino);
		RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

		if (opened) {
			/* File is opened, deleted it at last close */
			snprintf(k, KLEN, "%llu.opened_and_deleted", ino);
			snprintf(v, VLEN, "1");
			RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);
		}

		/* Remove all associated xattr */
		deleted = true;
	} else {
		for (i = 0; i < size ; i++)
			if (parent[i] == *dir) {
				/* In this list mgmt, setting value 0
				 * will make it ignored as str is rebuilt */
				parent[i] = 0;
				break;
			}
		snprintf(k, KLEN, "%llu.parentdir", ino);
		RC_WRAP_LABEL(rc, aborted, kvsns_parentlist2str,
			      parent, size, v);
		RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

		RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &ino_stat,
			 STAT_CTIME_SET|STAT_DECR_LINK);
		RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, &ino, &ino_stat);
	}

	snprintf(k, KLEN, "%llu.dentries.%s",
		 *dir, name);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	/* if object is a link, delete the link content as well */
	if ((ino_stat.st_mode & S_IFLNK) == S_IFLNK) {
		snprintf(k, KLEN, "%llu.link", ino);
		RC_WRAP_LABEL(rc, aborted, kvsal_del, k);
	}

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &dir_stat,
		      STAT_MTIME_SET|STAT_CTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, dir, &dir_stat);

	RC_WRAP(kvsal_end_transaction);

	/* Call to object store : do not mix with metadata transaction */
	if (!opened)
		RC_WRAP(extstore_del, &ino);

	if (deleted)
		RC_WRAP(kvsns_remove_all_xattr, cred, &ino);
	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

int kvsns_rename(kvsns_cred_t *cred,  kvsns_ino_t *sino,
		 char *sname, kvsns_ino_t *dino, char *dname)
{
	int rc = 0;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino = 0LL;
	kvsns_ino_t parent[KVSAL_ARRAY_SIZE];
	struct stat sino_stat;
	struct stat dino_stat;
	int size = 0;
	int i = 0;

	if (!cred || !sino || !sname || !dino || !dname)
		return -EINVAL;

	memset(parent, 0, KVSAL_ARRAY_SIZE*sizeof(kvsns_ino_t));
	memset(&sino_stat, 0, sizeof(sino_stat));
	memset(&dino_stat, 0, sizeof(dino_stat));

	RC_WRAP(kvsns_access, cred, sino, KVSNS_ACCESS_WRITE);

	RC_WRAP(kvsns_access, cred, dino, KVSNS_ACCESS_WRITE);

	rc = kvsns_lookup(cred, dino, dname, &ino);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP(kvsns_get_stat, sino, &sino_stat);
	if (*sino != *dino)
		RC_WRAP(kvsns_get_stat, dino, &dino_stat);

	RC_WRAP(kvsns_lookup, cred, sino, sname, &ino);

	snprintf(k, KLEN, "%llu.parentdir", ino);
	RC_WRAP(kvsal_get_char, k, v);

	size = KVSAL_ARRAY_SIZE;
	RC_WRAP(kvsns_str2parentlist, parent, &size, v);
	for (i = 0; i < size ; i++)
		if (parent[i] == *sino) {
			parent[i] = *dino;
			break;
		}

	RC_WRAP(kvsal_begin_transaction);
	snprintf(k, KLEN, "%llu.dentries.%s",
		 *sino, sname);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	snprintf(k, KLEN, "%llu.dentries.%s",
		 *dino, dname);
	snprintf(v, VLEN, "%llu", ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	snprintf(k, KLEN, "%llu.parentdir", ino);
	RC_WRAP_LABEL(rc, aborted, kvsns_parentlist2str, parent, size, v);
	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &sino_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, sino, &sino_stat);
	if (*sino != *dino) {
		RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &dino_stat,
			 STAT_CTIME_SET|STAT_MTIME_SET);
		RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, dino, &dino_stat);
	}
	RC_WRAP(kvsal_end_transaction);
	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}


int kvsns_mr_proper(void)
{
	int rc;
	char pattern[KLEN];
	kvsal_item_t items[KVSAL_ARRAY_SIZE];
	int i;
	int size;
	kvsal_list_t list;

	snprintf(pattern, KLEN, "*");

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

	rc = kvsal_fetch_list(pattern, &list);
	if (rc < 0)
		return rc;

	return 0;
}

