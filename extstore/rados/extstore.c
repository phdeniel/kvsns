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


#include <kvsns/extstore.h>
#include <rados/librados.h>

#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc != 0)	\
		return __rc; })

#define RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	if (__rc != 0)        \
		goto __label; })

#define CEPH_CONFIG_DEFAULT "/etc/ceph/ceph.conf"

static char pool[MAXNAMLEN];
__thread rados_t cluster;

static void build_objid(kvsns_ino_t ino, char *objid, int objidlen)
{
	if (!ino)
		return;

	snprintf(objid, objidlen, "kvsns.%llu", ino);
}

int extstore_create(kvsns_ino_t object)
{
	int rc;
	rados_ioctx_t io;
	char objid[MAXNAMLEN];

	build_objid(object, objid, MAXNAMLEN);

	rc = rados_ioctx_create(cluster, pool, &io);
	if (rc < 0)
		return rc;

	rc = rados_write(io, objid, "", 0, 0);
	if (rc < 0)
		return rc;

	rados_ioctx_destroy(io);
	return 0;
}

int extstore_attach(kvsns_ino_t *ino, char *objid, int objid_len)
{
	/* In the case of RADOS, nothing is to be done for the
	 * object store manages its md */
	return 0;
}

int extstore_init(struct collection_item *cfg_items)
{
	struct collection_item *item;
	int rc;
	char clustername[MAXNAMLEN];
	char user[MAXNAMLEN];
	char ceph_conf[MAXPATHLEN];

	if (cfg_items == NULL)
		return -EINVAL;

	/* Get config from ini file */
	item = NULL;
	rc = get_config_item("rados", "pool",
			      cfg_items, &item);
	if (rc != 0)
		return -rc;
	if (item == NULL)
		return -EINVAL;

	strncpy(pool, get_string_config_value(item, NULL),
		MAXNAMLEN);

	item = NULL;
	rc = get_config_item("rados", "cluster",
			      cfg_items, &item);
	if (rc != 0)
		return -rc;
	if (item == NULL)
		return -EINVAL;
	strncpy(clustername, get_string_config_value(item, NULL),
		MAXNAMLEN);

	item = NULL;
	rc = get_config_item("rados", "user",
			      cfg_items, &item);
	if (rc != 0)
		return -rc;
	if (item == NULL)
		return -EINVAL;
	strncpy(user, get_string_config_value(item, NULL),
		MAXNAMLEN);

	item = NULL;
	rc = get_config_item("rados", "config",
			      cfg_items, &item);
	if (rc != 0)
		return -rc;
	if (item == NULL)
		strncpy(ceph_conf, CEPH_CONFIG_DEFAULT, MAXPATHLEN);
	else
		strncpy(ceph_conf, get_string_config_value(item, NULL),
			MAXPATHLEN);

	/* Rados init */
	rc = rados_create2(&cluster, clustername, user, 0LL);
	if (rc < 0)
		return rc;

	rc = rados_conf_read_file(cluster, ceph_conf);
	if (rc < 0)
		return rc;

	/* Connect to the cluster */
	rc = rados_connect(cluster);
	if (rc < 0)
		return rc;

	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	int rc;
	rados_ioctx_t io;
	char objid[MAXNAMLEN];

	if (!ino)
		return -EINVAL;

	build_objid(*ino, objid, MAXNAMLEN);

	rc = rados_ioctx_create(cluster, pool, &io);
	if (rc < 0)
		return rc;

	rc = rados_remove(io, objid);
	if (rc < 0)
		return rc;

	rados_ioctx_destroy(io);
	return 0;
}

int extstore_read(kvsns_ino_t *ino,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	int rc;
	rados_ioctx_t io;
	char objid[MAXNAMLEN];
	uint64_t size;
	time_t mtime;
	int read;

	if (!ino)
		return -EINVAL;

	build_objid(*ino, objid, MAXNAMLEN);

	rc = rados_ioctx_create(cluster, pool, &io);
	if (rc < 0)
		return rc;

	read = rados_read(io, objid, buffer, buffer_size, offset);
	if (read < 0)
		return read;

	rc = rados_stat(io, objid, &size, &mtime);
	if (rc < 0)
		return rc;

	rados_ioctx_destroy(io);

	stat->st_size = size;
	stat->st_mtime = mtime;
	stat->st_atime = mtime; /* @todo bug ?*/

	return read;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	int rc;
	rados_ioctx_t io;
	char objid[MAXNAMLEN];
	uint64_t size;
	time_t mtime;
	int written;

	if (!ino)
		return -EINVAL;

	build_objid(*ino, objid, MAXNAMLEN);

	rc = rados_ioctx_create(cluster, pool, &io);
	if (rc < 0)
		return rc;

	written = rados_write(io, objid, buffer, buffer_size, offset);
	if (written < 0)
		return written;

	rc = rados_stat(io, objid, &size, &mtime);
	if (rc < 0)
		return rc;

	rados_ioctx_destroy(io);

	stat->st_size = size;
	stat->st_mtime = mtime;
	stat->st_atime = mtime; /* @todo bug ?*/

	return written;
}


int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      bool on_obj_store,
		      struct stat *stat)
{
	int rc;
	rados_ioctx_t io;
	char objid[MAXNAMLEN];
	uint64_t size;
	time_t mtime;

	if (!ino || !stat)
		return -EINVAL;

	build_objid(*ino, objid, MAXNAMLEN);

	rc = rados_ioctx_create(cluster, pool, &io);
	if (rc < 0)
		return rc;

	rc = rados_trunc(io, objid, (uint64_t)filesize);
	if (rc < 0)
		return rc;

	rc = rados_stat(io, objid, &size, &mtime);
	if (rc < 0)
		return rc;

	rados_ioctx_destroy(io);

	stat->st_size = size;
	stat->st_mtime = mtime;
	stat->st_atime = mtime; /* @todo bug ?*/

	return 0;
}

int extstore_getattr(kvsns_ino_t *ino,
		     struct stat *stat)
{
	int rc;
	rados_ioctx_t io;
	char objid[MAXNAMLEN];
	uint64_t size;
	time_t mtime;

	if (!ino || !stat)
		return -EINVAL;

	build_objid(*ino, objid, MAXNAMLEN);

	rc = rados_ioctx_create(cluster, pool, &io);
	if (rc < 0)
		return rc;

	rc = rados_stat(io, objid, &size, &mtime);
	if (rc < 0)
		return rc;

	rados_ioctx_destroy(io);

	stat->st_size = size;
	stat->st_mtime = mtime;
	stat->st_atime = mtime; /* @todo bug ?*/

	return 0;
}
