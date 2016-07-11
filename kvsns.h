#ifndef _KVSNS_H
#define _KVSNS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include "kvshl/kvshl.h"

#define KVSNS_ROOT_INODE 2LL

typedef unsigned long long int kvsns_ino_t;

typedef struct kvsns_cred__ 
{
	uid_t uid;
	gid_t gid;
} kvsns_cred_t;

typedef struct kvsns_entry_
{
	char name[MAXNAMLEN];
	kvsns_ino_t inode;
	struct stat stats;
} kvsns_dentry_t;

int kvsns_start(void);

int kvsns_init_root(void);

int kvsns_next_inode(kvsns_ino_t *ino);

int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir);

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name);

int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 kvsns_ino_t *myino);

int kvsns_readdir(kvsns_cred_t *cred, kvsns_ino_t *dirt, int offset, 
		  kvsns_dentry_t *dirent, int *size);

int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent);

int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat);


#endif
