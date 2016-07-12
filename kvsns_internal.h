#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "kvsns.h"
#include <string.h>

int kvsns_next_inode(kvsns_ino_t *ino);
int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str);
int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str);
