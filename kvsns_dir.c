#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kvsns.h"
#include "kvsns_internal.h"
#include "kvshl/kvshl.h"

int kvsns_start(void)
{
	int rc;

	rc = kvshl_init();
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_init_root()
{
	int rc;
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;
	kvsns_cred_t cred;
	kvsns_ino_t ino;

	cred.uid= 0;
	cred.gid = 0;
	ino = KVSNS_ROOT_INODE;

	kvshl_begin_transaction();

	snprintf(k, KLEN, "%llu.parentdir", ino);
	snprintf(v, VLEN, "%llu|", ino);

	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_mode = S_IFDIR|0755;
	bufstat.st_ino = KVSNS_ROOT_INODE;
	bufstat.st_nlink = 2;
	bufstat.st_uid = 0;
	bufstat.st_gid = 0;
	bufstat.st_atim.tv_sec = 0;
	bufstat.st_mtim.tv_sec = 0;
	bufstat.st_ctim.tv_sec = 0;

	snprintf(k, KLEN, "%llu.stat", ino);
	rc = kvshl_set_stat(k, &bufstat);
	if (rc != 0)
		return rc;

	kvshl_end_transaction();
	return 0;
}



int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir)
{
	return kvsns_create_entry(cred, parent, name,
				  mode, newdir, KVSNS_DIR);
}

int kvsns_symlink(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  char *content, kvsns_ino_t *newlnk)
{
	int rc;
	char k[KLEN];

	if (!cred || !parent || ! name || !content || !newlnk)
		return -EINVAL;

	rc = kvsns_create_entry(cred, parent, name,
				0, newlnk, KVSNS_SYMLINK);
	if (rc)
		return rc;

	snprintf(k, KLEN, "%llu.link", *newlnk);
	rc = kvshl_set_char(k, content);
	
	return rc;
}

int kvsns_readlink(kvsns_cred_t *cred, kvsns_ino_t *lnk, 
		  char *content, int *size) 
{
	int rc;
	char k[KLEN];
	char v[KLEN];

	if (!cred || !lnk || !content || !size)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.link", *lnk);
	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	strncpy(content, v, *size);
	*size = strnlen(v, VLEN); 
	
	return 0;
}

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name)
{
	int rc;
	char k[KLEN];
	kvsns_ino_t ino;

	if (!cred || !parent || !name)
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

	snprintf(k, KLEN, "%llu.parentdir", ino);

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

int kvsns_readdir(kvsns_cred_t *cred, kvsns_ino_t *dir, int offset, 
		  kvsns_dentry_t *dirent, int *size)
{
	int rc;
	char pattern[KLEN];
	char v[VLEN];
	kvshl_item_t *items;
	int i; 
	kvsns_ino_t ino;

	if (!cred || !dir || !dirent || !size)
		return -EINVAL;

	items = (kvshl_item_t *)malloc(*size*sizeof(kvshl_item_t));
	if (items == NULL)
		return -ENOMEM;

	kvshl_begin_transaction();

	snprintf(pattern, KLEN, "%llu.dentries.*", *dir);
	rc = kvshl_get_list(pattern, offset, size, items);
	if (rc < 0)
		return rc;

	for (i=0; i < *size ; i++) {
		sscanf(items[i].str, "%llu.dentries.%s\n", 
		       &ino, dirent[i].name);

		rc = kvshl_get_char(items[i].str, v);
		if (rc != 0)
			return rc;
		sscanf(v, "%llu", &dirent[i].inode);		

		rc = kvsns_getattr(cred, &dirent[i].inode, &dirent[i].stats);
		if (rc != 0)
			return rc;
	} 

	kvshl_end_transaction();

	return 0;
}

int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		kvsns_ino_t *ino)
{
	int rc;
	char k[KLEN];
	char v[VLEN];

	if (!cred || !parent || !name || !ino)
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

	if (!cred || !dir || !parent)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.parentdir", 
		 *dir);

	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	sscanf(v, "%llu|", parent);

	return 0;
}

int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat) 
{
	char k[KLEN];

	if (!cred || !ino || !buffstat)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvshl_get_stat(k, buffstat);
}

int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *setstat, int statflag) 
{
	char k[KLEN];
	struct stat buffstat;
	int rc;

	if (!cred || !ino || !setstat)
		return -EINVAL;

	if (statflag == 0)
		return 0; /* Nothing to do */

	snprintf(k, KLEN, "%llu.stat", *ino);
	rc = kvshl_get_stat(k, &buffstat);
	if (rc != 0)
		return rc;

	if (statflag & STAT_MODE_SET)
		buffstat.st_mode = setstat->st_mode;

	if (statflag & STAT_UID_SET)
		buffstat.st_uid = setstat->st_uid;
		
	if (statflag & STAT_GID_SET)
		buffstat.st_gid = setstat->st_gid;
		
	if (statflag & STAT_SIZE_SET)
		buffstat.st_size = setstat->st_size;
		
	if (statflag & STAT_ATIME_SET) {
		buffstat.st_atim.tv_sec = setstat->st_atim.tv_sec;
		buffstat.st_atim.tv_nsec = setstat->st_atim.tv_nsec;
	}		

	if (statflag & STAT_MTIME_SET) {
		buffstat.st_mtim.tv_sec = setstat->st_mtim.tv_sec;
		buffstat.st_mtim.tv_nsec = setstat->st_mtim.tv_nsec;
	}		

	if (statflag & STAT_CTIME_SET) {
		buffstat.st_ctim.tv_sec = setstat->st_ctim.tv_sec;
		buffstat.st_ctim.tv_nsec = setstat->st_ctim.tv_nsec;
	}		

	return kvshl_set_stat(k, &buffstat);
}

int kvsns_link(kvsns_cred_t *cred, kvsns_ino_t *ino, kvsns_ino_t *dino, char *dname)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t tmpino;
	kvsns_ino_t parent[KVSNS_ARRAY_SIZE];
	int size ;
	struct stat buffstat;

	if (!cred || !ino || !dino || !dname)
		return -EINVAL;

	rc = kvsns_lookup(cred, dino, dname, &tmpino);
	if (rc == 0)
		return -EEXIST;

	snprintf(k, KLEN, "%llu.parentdir", *ino);
	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu|", *dino);
	strcat(v,k);
 
	snprintf(k, KLEN, "%llu.parentdir", *ino);
	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.stat", *ino);
	rc = kvshl_get_stat(k, &buffstat);
	if (rc != 0)
		return rc;

	buffstat.st_nlink += 1;
	buffstat.st_ctim.tv_sec = time(NULL);
	rc = kvshl_set_stat(k, &buffstat);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *dino, dname);
	snprintf(v, VLEN, "%llu", *ino);
	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *dir, char *name)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino;
	kvsns_ino_t parent[KVSNS_ARRAY_SIZE];
	int size;
	int i;
	struct stat buffstat;

	if (!cred || !dir || !name)
		return -EINVAL;

	rc = kvsns_lookup(cred, dir, name, &ino);
	if (rc !=0)
		return rc;

	kvshl_begin_transaction();
	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *dir, name);

	rc = kvshl_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", ino);
	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	size = KVSNS_ARRAY_SIZE;
	kvsns_str2parentlist(parent, &size, v);
	if (size == 1) {
		rc = kvshl_del(k);
		if (rc != 0)
			return rc;

		snprintf(k, KLEN, "%llu.stat", ino);
		rc = kvshl_del(k);
		if (rc != 0)
			return rc;
	} else {
		for(i=0; i < size ; i++)
			if (parent[i] == *dir) {
				parent[i] = 0;
				break;
			}
		kvsns_parentlist2str(parent, size, v);
		rc = kvshl_set_char(k, v);
		if (rc != 0)
			return rc;

		snprintf(k, KLEN, "%llu.stat", ino);
		rc = kvshl_get_stat(k, &buffstat);
		if (rc != 0)
			return rc;

		buffstat.st_nlink -= 1;
		buffstat.st_ctim.tv_sec = time(NULL);

		rc = kvshl_set_stat(k, &buffstat);
		if (rc != 0)
			return rc;

	}
	kvshl_end_transaction();
	
	return 0;
}

int kvsns_rename(kvsns_cred_t *cred,  kvsns_ino_t *sino, char *sname, kvsns_ino_t *dino, char *dname)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino;
	kvsns_ino_t parent[KVSNS_ARRAY_SIZE];
	int size;
	struct stat buffstat;
	int i;

	if (!cred || !sino || !sname || !dino || !dname)
		return -EINVAL;

	rc = kvsns_lookup(cred, dino, dname, &ino);
	if (rc ==0)
		return -EEXIST;

	rc = kvsns_lookup(cred, sino, sname, &ino);
	if (rc !=0)
		return rc; 

	kvshl_begin_transaction();

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *sino, sname);
	rc = kvshl_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *dino, dname);
	snprintf(v, VLEN, "%llu", ino);
	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", ino);
	rc = kvshl_get_char(k, v);
	if (rc != 0)
		return rc;

	size = KVSNS_ARRAY_SIZE;
	kvsns_str2parentlist(parent, &size, v);
	for(i=0; i < size ; i++)
		if (parent[i] == *sino) {
			parent[i] = *dino;
			break;
		}
	kvsns_parentlist2str(parent, size, v);

	rc = kvshl_set_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.stat", ino);
	rc = kvshl_get_stat(k, &buffstat);
	if (rc != 0)
		return rc;

	buffstat.st_ctim.tv_sec = time(NULL);

	rc = kvshl_set_stat(k, &buffstat);
	if (rc != 0)
		return rc;

	kvshl_end_transaction();

	return 0;
}
