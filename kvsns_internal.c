#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "kvsns.h"
#include <string.h>
#include "kvsns.h"
#include "kvsns_internal.h"

int kvsns_next_inode(kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -EINVAL;

	rc = kvsal_incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str)
{
	char *token;
	char *rest = str;
	int maxsize = *size;
	int pos = 0;

	if (!inolist || !str || !size)
		return -EINVAL;

	while((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%llu", &inolist[pos++]);

		if (pos == maxsize)
			break;
	}

	*size = pos;

	return 0;
}

int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str)
{
	int i;
	char tmp[VLEN];

	if (!inolist || !str)
		return -EINVAL;

	strcpy(str, "");

	for (i=0; i < size ; i++)
		if (inolist[i] != 0LL) {
			snprintf(tmp, VLEN, "%llu|", inolist[i]);
			strcat(str, tmp);
		}
	
	return 0;	
}

int kvsns_update_stat(kvsns_ino_t *ino, int flags)
{
	struct timeval t;
	struct stat stat;
	char k[KLEN];
	int rc;

	if (!ino)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	snprintf(k, KLEN, "%llu.stat", *ino);
	rc = kvsal_get_stat(k, &stat);
	if (rc != 0)
		return rc;

	if (flags & STAT_ATIME_SET) {
		stat.st_atim.tv_sec = t.tv_sec;
		stat.st_atim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_MTIME_SET) {
		stat.st_mtim.tv_sec = t.tv_sec;
		stat.st_mtim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_CTIME_SET) {
		stat.st_ctim.tv_sec = t.tv_sec;
		stat.st_ctim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_INCR_LINK)
		stat.st_nlink += 1;

	if (flags & STAT_DECR_LINK) {
		if (stat.st_nlink == 1)
			return -EINVAL; 
	    
		stat.st_nlink -= 1;
	}

	return kvsal_set_stat(k, &stat);
}

int kvsns_create_entry(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
			mode_t mode, kvsns_ino_t *new_entry, enum kvsns_type type)
{
	int rc;
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;
	struct timeval t;

	if (!cred || !parent || !name || !new_entry)
		return -EINVAL;

	rc = kvsns_lookup(cred, parent, name, new_entry);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP(rc, kvsns_next_inode, new_entry);

	snprintf(k, KLEN, "%llu.dentries.%s", 
		 *parent, name);
	snprintf(v, VLEN, "%llu", *new_entry);
	
	RC_WRAP(rc, kvsal_set_char, k, v);

	snprintf(k, KLEN, "%llu.parentdir", *new_entry);
	snprintf(v, VLEN, "%llu|", *parent);

	RC_WRAP(rc, kvsal_set_char, k, v);

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_uid = getuid(); 
	bufstat.st_gid = getgid(); 
	bufstat.st_ino = *new_entry; 

       if (gettimeofday(&t, NULL) != 0)
                return -1;

        bufstat.st_atim.tv_sec = t.tv_sec;
        bufstat.st_atim.tv_nsec = 1000 * t.tv_usec;

	bufstat.st_mtim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_mtim.tv_nsec = bufstat.st_atim.tv_nsec;

	bufstat.st_ctim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_ctim.tv_nsec = bufstat.st_atim.tv_nsec;

	switch(type) {
	case KVSNS_DIR:
		bufstat.st_mode = S_IFDIR|mode;
		bufstat.st_nlink = 2;
		break;

	case KVSNS_FILE:
		bufstat.st_mode = S_IFREG|mode;
		bufstat.st_nlink = 1;
		break;

	case KVSNS_SYMLINK:
		bufstat.st_mode = S_IFLNK|mode;
		bufstat.st_nlink = 1;
		break;

	default:
		return -EINVAL;
	}

	snprintf(k, KLEN, "%llu.stat", *new_entry);
	RC_WRAP(rc, kvsal_set_stat, k, &bufstat);

	RC_WRAP(rc, kvsns_update_stat, parent, STAT_CTIME_SET|STAT_MTIME_SET);

	return 0;
}


/* Access routines */
static int kvsns_access_check(kvsns_cred_t *cred, struct stat *stat, int flags)
{
	int check = 0;

	if (!stat || !cred)
		return -EINVAL;

	/* Root's superpowers */
	if (cred->uid == KVSNS_ROOT_UID)
		return 0;

	if (cred->uid == stat->st_uid ) {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_OWNER_READ;
		
		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_OWNER_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_OWNER_EXEC;
	} else if (cred->gid == stat->st_gid) {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_GROUP_READ;
		
		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_GROUP_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_GROUP_EXEC;
	} else {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_OTHER_READ;
		
		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_OTHER_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_OTHER_EXEC;
	}

	if ((check & stat->st_mode ) != check)
		return -EPERM;
	else	
		return 0;

	/* Should not be reached */
	return -EPERM;
}

int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags) 
{
	struct stat stat;
	int rc;

	if (!cred || !ino)
		return -EINVAL;

	RC_WRAP(rc, kvsns_getattr, cred, ino, &stat);

	return kvsns_access_check(cred, &stat, flags);
} 
	
