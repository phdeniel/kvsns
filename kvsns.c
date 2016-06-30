#include "kvsns.h"
#include "kvshl/kvshl.h"

int kvsns_init(void)
{
	int rc;

	rc = kvshl_init();
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_next_inode(kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -1;

	rc = kvshl_incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_mkdir(kvsns_ino_t *parent, char *name,
		kvsns_ino_t *newdir)
{
	return 0;
}
