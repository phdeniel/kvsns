#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
		return -EINVAL;

	rc = kvshl_incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		kvsns_ino_t *newdir)
{
	int rc;
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;

	if (!parent || !name || !newdir)
		return -EINVAL;

	rc = kvsns_lookup(cred, parent, name, newdir);
	if (rc == 0)
		return -EEXIST;

	rc = kvsns_next_inode(newdir);
	if (rc != 0)
		return rc;

	kvshl_begin_transaction();
	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);
	snprintf(v, VLEN, "%llu", *newdir);
	
	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", *newdir);
	snprintf(v, VLEN, "%llu", *parent);

	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_mode = S_IFDIR|0755;
	bufstat.st_nlink = 2;
	bufstat.st_uid = getuid(); 
	bufstat.st_gid = getgid(); 
	bufstat.st_atim.tv_sec = time(NULL);
	bufstat.st_mtim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_ctim.tv_sec = bufstat.st_atim.tv_sec;

	snprintf(k, KLEN, "%llu.stat", *newdir);
	rc = kvshl_set_stat(k, &bufstat);
	if (rc != 0)
		return rc;

	kvshl_end_transaction();
	return 0;
}

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name)
{
	int rc;
	char k[KLEN];
	kvsns_ino_t ino;

	if (!parent || !name)
		return -EINVAL; 
	
	rc = kvsns_lookup(cred, parent, name, &ino);
	if (rc != 0)
		return -rc;

	snprintf(k, KLEN, "%llu.dentries.*", ino);
	rc = kvshl_get_list_size(k);
	if (rc > 0)
		return -ENOTEMPTY;

	kvshl_begin_transaction();
	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);

	rc = kvshl_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", &ino);

	rc = kvshl_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.stat", ino);
	rc = kvshl_del(k);
	if (rc != 0)
		return rc;

	kvshl_end_transaction();
	return 0;
}

int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		kvsns_ino_t *ino)
{
	int rc;
	char k[KLEN];
	char v[VLEN];

	if (!parent || !name || !ino)
		return -EINVAL; 

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);

	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	sscanf(v, "%llu", ino);

	return 0;
}

int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent)
{
	int rc;
	char k[KLEN];
	char v[VLEN];

	if (!dir || !parent)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.parentdir", 
		 *dir);

	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	sscanf(v, "%llu", parent);

	return 0;
}

int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat) 
{
	char k[KLEN];

	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvshl_get_stat(k, buffstat);
}
