/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* extstore.c
 * KVSNS: implement a dummy object store inside a POSIX directory
 */

#include <sys/time.h> /* for gettimeofday */
#include <ini_config.h>
#include <kvsns/extstore.h>

static char store_root[MAXPATHLEN];

static int build_extstore_path(kvsns_ino_t object,
			       char *extstore_path,
			       size_t pathlen)
{
	if (!extstore_path)
		return -1;

	return snprintf(extstore_path, pathlen, "%s/inum=%llu",
			store_root, (unsigned long long)object);
}

static int extstore_consolidate_attrs(kvsns_ino_t *ino, struct stat *filestat)
{
	struct stat extstat;
	char storepath[MAXPATHLEN];
	int rc;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = lstat(storepath, &extstat);
	if (rc < 0) {
		if (errno == ENOENT)
			return 0; /* No data written yet */
		else
			return -errno;
	}

	filestat->st_mtime = extstat.st_mtime;
	filestat->st_atime = extstat.st_atime;
	filestat->st_size = extstat.st_size;
	filestat->st_blksize = extstat.st_blksize;
	filestat->st_blocks = extstat.st_blocks;

	return 0;
}

int extstore_attach(kvsns_ino_t *ino, char *objid, int objid_len)
{
	return -ENOTSUP;
}

int extstore_create(kvsns_ino_t object)
{
	return 0;
}

int extstore_init(struct collection_item *cfg_items)
{
	struct collection_item *item;
	int rc;

	item = NULL;
	rc = get_config_item("posix_store", "root_path",
			     cfg_items, &item);
	if (rc != 0)
		return -rc;

	if (item == NULL)
		return -EINVAL;

	strncpy(store_root, get_string_config_value(item, NULL),
		MAXPATHLEN);

	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	char storepath[MAXPATHLEN];
	int rc;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = unlink(storepath);
	if (rc) {
		if (errno == ENOENT)
			return 0;

		return -errno;
	}

	return 0;
}

int extstore_read(kvsns_ino_t *ino,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc;
	int fd;
	ssize_t read_bytes;
	struct stat storestat;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	fd = open(storepath, O_CREAT|O_RDONLY|O_SYNC, 0755);
	if (fd < 0) {
		return -errno;
	}

	read_bytes = pread(fd, buffer, buffer_size, offset);
	if (read_bytes < 0) {
		close(fd);
		return -errno;
	}

	rc = fstat(fd, &storestat);
	if (rc < 0)
		return -errno;

	stat->st_mtime = storestat.st_mtime;
	stat->st_size = storestat.st_size;
	stat->st_blocks = storestat.st_blocks;
	stat->st_blksize = storestat.st_blksize;

	rc = close(fd);
	if (rc < 0)
		return -errno;

	return read_bytes;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc;
	int fd;
	ssize_t written_bytes;
	struct stat storestat;


	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	fd = open(storepath, O_CREAT|O_WRONLY|O_SYNC, 0755);
	if (fd < 0)
		return -errno;

	written_bytes = pwrite(fd, buffer, buffer_size, offset);
	if (written_bytes < 0) {
		close(fd);
		return -errno;
	}

	rc = fstat(fd, &storestat);
	if (rc < 0) {
		close(fd);
		return -errno;
	}

	stat->st_mtime = storestat.st_mtime;
	stat->st_size = storestat.st_size;
	stat->st_blocks = storestat.st_blocks;
	stat->st_blksize = storestat.st_blksize;

	rc = close(fd);
	if (rc < 0)
		return -errno;

	*fsal_stable = true;
	return written_bytes;
}


int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      bool on_obj_store,
		      struct stat *stat)
{
	int rc;
	char storepath[MAXPATHLEN];
	struct timeval t;

	if (!ino || !stat)
		return -EINVAL;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = truncate(storepath, filesize);
	if (rc == -1) {
		if (errno == ENOENT) {
			/* File does not exist in data store
			 * it is a mere MD creature */
			stat->st_size = filesize;

			if (gettimeofday(&t, NULL) != 0)
				return -errno;

			stat->st_ctim.tv_sec = t.tv_sec;
			stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
			stat->st_mtim.tv_sec = stat->st_ctim.tv_sec;
			stat->st_mtim.tv_nsec = stat->st_ctim.tv_nsec;

			return 0;
		} else
			return -errno;
	}

	rc = extstore_consolidate_attrs(ino, stat);
	if (rc < 0)
		return rc;

	return 0;
}

