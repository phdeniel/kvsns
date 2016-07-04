#ifndef _KVSNS_H
#define _KVSNS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "kvshl/kvshl.h"

typedef unsigned long long int kvsns_ino_t;
typedef struct kvsns_cred__ 
{
	uid_t uid;
	gid_t gid;
} kvsns_cred_t;

int kvsns_init(void);

int kvsns_next_inode(kvsns_ino_t *ino);

int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		kvsns_ino_t *newdir);

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name);

int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 kvsns_ino_t *myino);

int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent);
int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat);

#endif
