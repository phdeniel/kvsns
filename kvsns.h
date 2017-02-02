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

#ifndef _KVSNS_H
#define _KVSNS_H

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
#define KVSNS_STORE_DEFAULT "/btrfs"
#define KVSNS_SERVER "KVSNS_SERVER"
#define KLEN 256
#define VLEN 256

#define STAT_MODE_SET	0x001
#define STAT_UID_SET	0x002
#define STAT_GID_SET	0x004
#define STAT_SIZE_SET	0x008
#define STAT_ATIME_SET	0x010
#define STAT_MTIME_SET	0x020
#define STAT_CTIME_SET	0x040
#define STAT_INCR_LINK  0x080
#define STAT_DECR_LINK  0x100

#define STAT_OWNER_READ		0400	/* Read by owner. */
#define STAT_OWNER_WRITE	0200	/* Write by owner. */
#define STAT_OWNER_EXEC		0100	/* Execute by owner. */
#define STAT_GROUP_READ		0040	/* Read by group. */
#define STAT_GROUP_WRITE	0020	/* Write by group. */
#define STAT_GROUP_EXEC		0010	/* Execute by group. */
#define STAT_OTHER_READ		0004	/* Read by other. */
#define STAT_OTHER_WRITE	0002	/* Write by other. */
#define STAT_OTHER_EXEC		0001	/* Execute by other. */

#define KVSNS_ACCESS_READ 	1
#define KVSNS_ACCESS_WRITE	2
#define KVSNS_ACCESS_EXEC	4

/* KVSAL related definitions and functions */


typedef unsigned long long int kvsns_ino_t;

/* KVSNS related definitions and functions */
typedef struct kvsns_cred__ {
	uid_t uid;
	gid_t gid;
} kvsns_cred_t;

typedef struct kvsns_fsstat_ {
	unsigned long nb_inodes;
} kvsns_fsstat_t;

typedef struct kvsns_dentry_ {
	char name[MAXNAMLEN];
	kvsns_ino_t inode;
	struct stat stats;
} kvsns_dentry_t;

typedef struct kvsns_open_owner_ {
	int pid;
	pthread_t thrid;
} kvsns_open_owner_t;

typedef struct kvsns_file_open_ {
	kvsns_ino_t ino;
	kvsns_open_owner_t owner;
	int flags;
} kvsns_file_open_t;

typedef struct kvsns_dir {
	kvsns_ino_t ino;
	kvsal_list_t list;
} kvsns_dir_t;

enum kvsns_type {
	KVSNS_DIR = 1,
	KVSNS_FILE = 2,
	KVSNS_SYMLINK = 3
};

typedef struct kvsns_xattr__ {
	char name[MAXNAMLEN];
} kvsns_xattr_t;

int kvsns_start(void);
int kvsns_stop(void);
void kvsns_set_debug(bool debug);
int kvsns_init_root(int openbar);
int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags);
int kvsns_creat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir);
int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir);
int kvsns_symlink(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  char *content, kvsns_ino_t *newlnk);
int kvsns_readlink(kvsns_cred_t *cred, kvsns_ino_t *link,
		  char *content, size_t *size);
int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name);
int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 kvsns_ino_t *myino);
int kvsns_opendir(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_dir_t *ddir);
int kvsns_readdir(kvsns_cred_t *cred, kvsns_dir_t *dir, off_t offset,
		  kvsns_dentry_t *dirent, int *size);
int kvsns_closedir(kvsns_dir_t *ddir);
int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent);
int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat);
int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *setstat,
		  int statflags);
int kvsns_link(kvsns_cred_t *cred, kvsns_ino_t *ino, kvsns_ino_t *dino,
	       char *dname);
int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);
int kvsns_rename(kvsns_cred_t *cred,  kvsns_ino_t *sino, char *sname,
		 kvsns_ino_t *dino, char *dname);
int kvsns_fsstat(kvsns_fsstat_t *stat);
int kvsns_get_root(kvsns_ino_t *ino);


int kvsns_open(kvsns_cred_t *cred, kvsns_ino_t *ino,
	       int flags, mode_t mode, kvsns_file_open_t *fd);
int kvsns_openat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 int flags, mode_t mode, kvsns_file_open_t *fd);
int kvsns_close(kvsns_file_open_t *fd);
ssize_t kvsns_write(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		    void *buf, size_t count, off_t offset);
ssize_t kvsns_read(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		   void *buf, size_t count, off_t offset);

/* Xattr */
int kvsns_setxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t size, int flags);
int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t *size);
int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset,
		  kvsns_xattr_t *list, int *size);
int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);
int kvsns_remove_all_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino);

/* For utility */
int kvsns_mr_proper(void);

#endif
