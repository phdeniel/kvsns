#include "kvshl/kvshl.h"

typedef unsigned long long int kvsns_ino_t;

int kvsns_init(void);

int kvsns_next_inode(kvsns_ino_t *ino);

int kvsns_mkdir(kvsns_ino_t *parent, char *name,
		kvsns_ino_t *newdir);
