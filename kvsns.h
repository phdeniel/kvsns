#ifndef _KVSNS_H
#define _KVSNS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include "kvsal/kvsal.h"

#define KVSNS_ROOT_INODE 2LL
#define KVSNS_ARRAY_SIZE 100
#define KVSNS_ROOT_UID 0

typedef unsigned long long int kvsns_ino_t;

#define STAT_MODE_SET	0x01
#define STAT_UID_SET	0x02
#define STAT_GID_SET	0x04
#define STAT_SIZE_SET	0x08
#define STAT_ATIME_SET	0x10
#define STAT_MTIME_SET	0x11
#define STAT_CTIME_SET	0x12

#define STAT_OWNER_READ       	0400    /* Read by owner.  */
#define STAT_OWNER_WRITE      	0200    /* Write by owner.  */
#define STAT_OWNER_EXEC       	0100    /* Execute by owner.  */
#define STAT_GROUP_READ       	0040    /* Read by group.  */
#define STAT_GROUP_WRITE      	0020    /* Write by group.  */
#define STAT_GROUP_EXEC	0010    /* Execute by group.  */
#define STAT_OTHER_READ       	0004    /* Read by other.  */
#define STAT_OTHER_WRITE      	0002    /* Write by other.  */
#define STAT_OTHER_EXEC	0001    /* Execute by other.  */

#define KVSNS_ACCESS_READ 	1
#define KVSNS_ACCESS_WRITE	2
#define KVSNS_ACCESS_EXEC	4

typedef struct kvsns_cred__ 
{
	uid_t uid;
	gid_t gid;
} kvsns_cred_t;

typedef struct kvsns_dentry_
{
	char name[MAXNAMLEN];
	kvsns_ino_t inode;
	struct stat stats;
} kvsns_dentry_t;

enum kvsns_type {
	KVSNS_DIR = 1,
	KVSNS_FILE = 2,
	KVSNS_SYMLINK = 3
};

typedef struct kvsns_xattr__
{
	char name[MAXNAMLEN];
} kvsns_xattr_t;

int kvsns_start(void);
int kvsns_init_root(void);
int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags); 
int kvsns_creat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir);
int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir);
int kvsns_symlink(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  char *content, kvsns_ino_t *newlnk);
int kvsns_readlink(kvsns_cred_t *cred, kvsns_ino_t *link, 
		  char *content, int *size); 
int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name);
int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 kvsns_ino_t *myino);
int kvsns_readdir(kvsns_cred_t *cred, kvsns_ino_t *dirt, int offset, 
		  kvsns_dentry_t *dirent, int *size);
int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent);
int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat);
int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *setstat, int statflags);
int kvsns_link(kvsns_cred_t *cred, kvsns_ino_t *ino, kvsns_ino_t *dino, char *dname);
int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);
int kvsns_rename(kvsns_cred_t *cred,  kvsns_ino_t *sino, char *sname, kvsns_ino_t *dino, char *dname);

int kvsns_setxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t size, int flags);
int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t size);
int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset, 
		  kvsns_xattr_t *list, int *size);
int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);

#endif
