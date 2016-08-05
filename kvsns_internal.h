#ifndef KVSNS_INTERNAL_H
#define KVSNS_INTERNAL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "kvsns.h"
#include <string.h>


int kvsns_next_inode(kvsns_ino_t *ino);
int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str);
int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str);
int kvsns_create_entry(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
			mode_t mode, kvsns_ino_t *newdir, enum kvsns_type type);
int kvsns_update_stat(kvsns_ino_t *ino, int flags);
int kvsns_delall_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino);
int kvsal_set_stat(char *k, struct stat *buf);
int kvsal_get_stat(char *k, struct stat *buf);


#endif
