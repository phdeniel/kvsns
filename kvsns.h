#ifndef _KVSNS_H
#define _KVSNS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "kvshl/kvshl.h"

typedef unsigned long long int kvsns_ino_t;

int kvsns_init(void);

int kvsns_next_inode(kvsns_ino_t *ino);

int kvsns_mkdir(kvsns_ino_t *parent, char *name,
		kvsns_ino_t *newdir);

int kvsns_lookup(kvsns_ino_t *parent, char *name,
		 kvsns_ino_t *myino);

int kvsns_lookupp(kvsns_ino_t *dir, kvsns_ino_t *parent);

#endif
