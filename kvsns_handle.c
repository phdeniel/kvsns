#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kvsns.h"
#include "kvsns_internal.h"


int kvsns_start(void)
{
	int rc;

	rc = kvsal_init();
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_init_root(int openbar)
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

	snprintf(k, KLEN, "%llu.parentdir", ino);
	snprintf(v, VLEN, "%llu|", ino);
	rc = kvsal_set_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "ino_counter");
	snprintf(v, VLEN, "3");
	rc = kvsal_set_char(k, v);
	if (rc != 0)
		return rc;

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	if (openbar != 0)
		bufstat.st_mode = S_IFDIR|0777;
	else
		bufstat.st_mode = S_IFDIR|0755;
	bufstat.st_ino = KVSNS_ROOT_INODE;
	bufstat.st_nlink = 2;
	bufstat.st_uid = 0;
	bufstat.st_gid = 0;
	bufstat.st_atim.tv_sec = 0;
	bufstat.st_mtim.tv_sec = 0;
	bufstat.st_ctim.tv_sec = 0;

	snprintf(k, KLEN, "%llu.stat", ino);
	rc = kvsal_set_stat(k, &bufstat);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_fsstat(kvsns_fsstat_t *stat)
{
	char k[KLEN];
	int rc;

	if (!stat)
		return -EINVAL;

	snprintf(k, KLEN, "*.stat");
	rc = kvsal_get_list_size(k);
	if (rc < 0 )
		return rc;

	stat->nb_inodes = rc;
	return 0;
}

int kvsns_get_root(kvsns_ino_t *ino)
{
	if (!ino)
		return -EINVAL;

	*ino = KVSNS_ROOT_INODE;
	return 0;
}

int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir)
{
	int rc;

	rc = kvsns_access(cred, parent, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	return kvsns_create_entry(cred, parent, name,
				  mode, newdir, KVSNS_DIR);
}

int kvsns_symlink(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  char *content, kvsns_ino_t *newlnk)
{
	int rc;
	char k[KLEN];
	struct stat bufstat;

	if (!cred || !parent || ! name || !content || !newlnk)
		return -EINVAL;

	rc = kvsns_access(cred, parent, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	rc = kvsns_create_entry(cred, parent, name,
				0, newlnk, KVSNS_SYMLINK);
	if (rc)
		return rc;

	snprintf(k, KLEN, "%llu.link", *newlnk);
	rc = kvsal_set_char(k, content);

	rc = kvsns_update_stat(parent, STAT_MTIME_SET|STAT_CTIME_SET);

	return rc;
}

int kvsns_readlink(kvsns_cred_t *cred, kvsns_ino_t *lnk, 
		  char *content, size_t *size) 
{
	int rc;
	char k[KLEN];
	char v[KLEN];

	/* No access check, a symlink's content is always readable */

	if (!cred || !lnk || !content || !size)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.link", *lnk);
	rc = kvsal_get_char(k, v);
	if (rc != 0)
		return rc;

	strncpy(content, v, *size);
	*size = strnlen(v, VLEN); 
	
	rc = kvsns_update_stat(lnk, STAT_ATIME_SET);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name)
{
	int rc;
	char k[KLEN];
	kvsns_ino_t ino;

	if (!cred || !parent || !name)
		return -EINVAL; 
	
	rc = kvsns_access(cred, parent, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	rc = kvsns_lookup(cred, parent, name, &ino);
	if (rc != 0)
		return -rc;

	snprintf(k, KLEN, "%llu.dentries.*", ino);
	rc = kvsal_get_list_size(k);
	if (rc > 0)
		return -ENOTEMPTY;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);

	rc = kvsal_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", ino);

	rc = kvsal_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.stat", ino);
	rc = kvsal_del(k);
	if (rc != 0)
		return rc;


	/* Remove all associated xattr */
	rc = kvsns_remove_all_xattr(cred, &ino);
	if (rc != 0)
		return rc;

	rc = kvsns_update_stat(parent, STAT_CTIME_SET|STAT_MTIME_SET);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_readdir(kvsns_cred_t *cred, kvsns_ino_t *dir, off_t offset, 
		  kvsns_dentry_t *dirent, int *size)
{
	int rc;
	char pattern[KLEN];
	char v[VLEN];
	kvsal_item_t *items;
	int i; 
	kvsns_ino_t ino;

	if (!cred || !dir || !dirent || !size)
		return -EINVAL;

	rc = kvsns_access(cred, dir, KVSNS_ACCESS_READ);
	if (rc != 0)
		return rc;

	items = (kvsal_item_t *)malloc(*size*sizeof(kvsal_item_t));
	if (items == NULL)
		return -ENOMEM;


	snprintf(pattern, KLEN, "%llu.dentries.*", *dir);
	rc = kvsal_get_list(pattern, (int)offset, size, items);
	if (rc < 0)
		return rc;

	for (i=0; i < *size ; i++) {
		sscanf(items[i].str, "%llu.dentries.%s\n", 
		       &ino, dirent[i].name);

		rc = kvsal_get_char(items[i].str, v);
		if (rc != 0)
			return rc;
		sscanf(v, "%llu", &dirent[i].inode);		

		rc = kvsns_getattr(cred, &dirent[i].inode, &dirent[i].stats);
		if (rc != 0)
			return rc;
	} 

	rc = kvsns_update_stat(dir, STAT_ATIME_SET);
	if (rc != 0)
		return rc;

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

	rc = kvsns_access(cred, parent, KVSNS_ACCESS_READ);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);

	rc = kvsal_get_char(k, v);
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

	rc = kvsns_access(cred, dir, KVSNS_ACCESS_READ);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", 
		 *dir);

	rc = kvsal_get_char(k, v);
	if (rc != 0)
		return rc;

	sscanf(v, "%llu|", parent);

	return 0;
}

int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *bufstat) 
{
	char k[KLEN];
	int rc;

	if (!cred || !ino || !bufstat)
		return -EINVAL;

	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvsal_get_stat(k, bufstat);
}

int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *setstat, int statflag) 
{
	char k[KLEN];
	struct stat bufstat;
	int rc;
	struct timeval t;

	if (!cred || !ino || !setstat)
		return -EINVAL;

	if (statflag == 0)
		return 0; /* Nothing to do */

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	rc = kvsns_access(cred, ino, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.stat", *ino);
	rc = kvsal_get_stat(k, &bufstat);
	if (rc != 0)
		return rc;

	/* ctime is to be updated if md are changed */
	bufstat.st_ctim.tv_sec = t.tv_sec;
	bufstat.st_ctim.tv_nsec = 1000 * t.tv_usec;
	
	if (statflag & STAT_MODE_SET)
		bufstat.st_mode = setstat->st_mode;

	if (statflag & STAT_UID_SET)
		bufstat.st_uid = setstat->st_uid;
		
	if (statflag & STAT_GID_SET)
		bufstat.st_gid = setstat->st_gid;
		
	if (statflag & STAT_SIZE_SET) {
		bufstat.st_size = setstat->st_size;
		bufstat.st_mtim.tv_sec = t.tv_sec;
		bufstat.st_mtim.tv_nsec = 1000 * t.tv_usec;
	}
		
	if (statflag & STAT_ATIME_SET) {
		bufstat.st_atim.tv_sec = setstat->st_atim.tv_sec;
		bufstat.st_atim.tv_nsec = setstat->st_atim.tv_nsec;
	}		

	if (statflag & STAT_MTIME_SET) {
		bufstat.st_mtim.tv_sec = setstat->st_mtim.tv_sec;
		bufstat.st_mtim.tv_nsec = setstat->st_mtim.tv_nsec;
	}		

	if (statflag & STAT_CTIME_SET) {
		bufstat.st_ctim.tv_sec = setstat->st_ctim.tv_sec;
		bufstat.st_ctim.tv_nsec = setstat->st_ctim.tv_nsec;
	}		

	return kvsal_set_stat(k, &bufstat);
}

int kvsns_link(kvsns_cred_t *cred, kvsns_ino_t *ino, kvsns_ino_t *dino, char *dname)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t tmpino;
	kvsns_ino_t parent[KVSNS_ARRAY_SIZE];
	int size ;

	if (!cred || !ino || !dino || !dname)
		return -EINVAL;

	rc = kvsns_access(cred, dino, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	rc = kvsns_lookup(cred, dino, dname, &tmpino);
	if (rc == 0)
		return -EEXIST;

	snprintf(k, KLEN, "%llu.parentdir", *ino);
	rc = kvsal_get_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu|", *dino);
	strcat(v,k);
 
	snprintf(k, KLEN, "%llu.parentdir", *ino);
	rc = kvsal_set_char(k, v);
	if (rc != 0)
		return rc;

		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *dino, dname);
	snprintf(v, VLEN, "%llu", *ino);
	rc = kvsal_set_char(k, v);
	if (rc != 0)
		return rc;

	kvsns_update_stat(ino, STAT_CTIME_SET|STAT_INCR_LINK);
	if (rc != 0)
		return rc;

	kvsns_update_stat(dino, STAT_CTIME_SET|STAT_MTIME_SET);
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

	if (!cred || !dir || !name)
		return -EINVAL;

	rc = kvsns_access(cred, dir, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	rc = kvsns_lookup(cred, dir, name, &ino);
	if (rc !=0)
		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *dir, name);

	rc = kvsal_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", ino);
	rc = kvsal_get_char(k, v);
	if (rc != 0)
		return rc;

	size = KVSNS_ARRAY_SIZE;
	kvsns_str2parentlist(parent, &size, v);
	if (size == 1) {
		rc = kvsal_del(k);
		if (rc != 0)
			return rc;

		snprintf(k, KLEN, "%llu.stat", ino);
		rc = kvsal_del(k);
		if (rc != 0)
			return rc;

		/* Remove all associated xattr */
		rc = kvsns_remove_all_xattr(cred, &ino);
		if( rc != 0)
			return rc;
	} else {
		for(i=0; i < size ; i++)
			if (parent[i] == *dir) {
				parent[i] = 0;
				break;
			}
		kvsns_parentlist2str(parent, size, v);
		rc = kvsal_set_char(k, v);
		if (rc != 0)
			return rc;

		rc = kvsns_update_stat(&ino, STAT_CTIME_SET|STAT_DECR_LINK);
		if (rc != 0)
			return rc;
	}

	rc = kvsns_update_stat(parent, STAT_MTIME_SET|STAT_CTIME_SET);
	if (rc != 0)
		return rc;	

	return 0;
}

int kvsns_rename(kvsns_cred_t *cred,  kvsns_ino_t *sino,
		 char *sname, kvsns_ino_t *dino, char *dname)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino;
	kvsns_ino_t parent[KVSNS_ARRAY_SIZE];
	int size;
	int i;

	if (!cred || !sino || !sname || !dino || !dname)
		return -EINVAL;

	rc = kvsns_access(cred, sino, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	rc = kvsns_access(cred, dino, KVSNS_ACCESS_WRITE);
	if (rc != 0)
		return rc;

	rc = kvsns_lookup(cred, dino, dname, &ino);
	if (rc ==0)
		return -EEXIST;

	rc = kvsns_lookup(cred, sino, sname, &ino);
	if (rc !=0)
		return rc; 


	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *sino, sname);
	rc = kvsal_del(k);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *dino, dname);
	snprintf(v, VLEN, "%llu", ino);
	rc = kvsal_set_char(k, v);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.parentdir", ino);
	rc = kvsal_get_char(k, v);
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

	rc = kvsal_set_char(k, v);
	if (rc != 0)
		return rc;

	rc = kvsns_update_stat(sino, STAT_CTIME_SET|STAT_MTIME_SET);
	if (rc != 0)
		return rc;

	if (*sino != *dino) {
		rc = kvsns_update_stat(dino, STAT_CTIME_SET|STAT_MTIME_SET);

		if (rc != 0)
			return rc;
	}
	return 0;
}
