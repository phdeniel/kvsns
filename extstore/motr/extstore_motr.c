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

#include <ini_config.h>
//#include <hiredis/hiredis.h>
#include <kvsns/extstore.h>
#include <kvsns/hashlib.h>
#include "../../motr/m0common.h"

#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc != 0)	\
		return __rc; })



#if 0
static int build_m0store_id(extstore_id_t *eid,
			    struct m0_uint128  *id)
{
	char k[KLEN];
	char v[VLEN];
	size_t klen;
	size_t vlen;
	int rc;


	snprintf(k, KLEN, "%llu.data", object);
	klen = strnlen(k, KLEN) + 1;
	vlen = VLEN;

	rc = m0kvs_get(k, klen, v, &vlen);
	if (rc != 0)
		return rc;

	*id = M0_ID_APP;
	id->u_lo = atoi(v);

	return 0;
	return -ENOTSUP;
}
#endif

enum update_stat_how {
	UP_ST_WRITE = 1,
	UP_ST_READ = 2,
	UP_ST_TRUNCATE = 3
};

static int update_stat(struct stat *stat, enum update_stat_how how,
		       off_t size)
{
	struct timeval t;

	if (!stat)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	switch (how) {
	case UP_ST_WRITE:
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
		stat->st_ctim = stat->st_mtim;
		if (size > stat->st_size) {
			stat->st_size = size;
			stat->st_blocks = size / DEV_BSIZE;
		}
		break;

	case UP_ST_READ:
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
		break;

	case UP_ST_TRUNCATE:
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
		stat->st_mtim = stat->st_ctim;
		stat->st_size = size;
		stat->st_blocks = size / DEV_BSIZE;
		break;

	default: /* Should not occur */
		return -EINVAL;
		break;
	}

	return 0;
}

int extstore_new_objectid(extstore_id_t *eid,
                          unsigned int seedlen,
                          char *seed)
{
        if (!eid || !seed)
                return -EINVAL;
	
	/* Ici il fait hacher une seed en m0128_t de motr qui va dans l'eid */
        /* Be careful about printf format: string with known length */
        eid->len = snprintf(eid->data, KLEN, "obj:%.*s", seedlen, seed);

        return 0;
}

static int eid2motr(extstore_id_t *eid, struct m0_uint128 *id)
{
	struct m0_fid *fid;
	hashbuff128_t hb;
	int rc;
	char strid[DATALEN];

	if (!eid || !id)
		return -EINVAL;

	rc = hashlib_murmur3_128("motr", eid->data, hb);
	if (rc)
		return -EINVAL; 

	memcpy(id, hb, sizeof(struct m0_uint128));

	id->u_lo |= M0_ID_APP.u_lo; /* Preserve reserved ids for the system */
	id->u_hi |= M0_ID_APP.u_hi; 
	fid = (struct m0_fid*)id;

	m0_fid_print(strid, DATALEN, fid);

	fprintf(stderr, "DBG: eid2motr: eid=%.*s id=%s\n",
		eid->len, eid->data, strid);

	return 0;
}

int extstore_create(extstore_id_t eid)
{
	struct m0_uint128 id;

	RC_WRAP(eid2motr, &eid, &id);
	RC_WRAP(m0store_create_object, id);

	/* Should handle MD */
	return 0;
}

int extstore_attach(extstore_id_t *eid)
{
	/* Should handle MD */
	return 0; 
}

int extstore_init(struct collection_item *cfg_items,
		  struct kvsal_ops *kvsalops)
{
	/* Init m0store */
	return m0init(cfg_items);
}

int extstore_del(extstore_id_t *eid)
{
	struct m0_uint128 id;
	int rc;

	RC_WRAP(eid2motr, eid, &id);

	rc = m0store_delete_object(id);
	if (rc) {
		if (errno == ENOENT)
			return 0;

		return -errno;
	}

	return 0;
}

int extstore_read(extstore_id_t *eid,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	ssize_t read_bytes;
	ssize_t bsize;
	struct m0_uint128 id;

	RC_WRAP(eid2motr, eid, &id);

	bsize = m0store_get_bsize(id);
	if (bsize < 0)
		return (int)bsize;

	read_bytes = m0store_pread(id, offset, buffer_size,
				   bsize, buffer);
	if (read_bytes < 0)
		return -1;

	RC_WRAP(update_stat, stat, UP_ST_READ, 0);

	return read_bytes;
}

int extstore_write(extstore_id_t *eid,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	ssize_t written_bytes;
	struct m0_uint128 id;
	ssize_t bsize;
	char k[KLEN];
	size_t klen, vlen;
	struct stat motr_stat;

	RC_WRAP(eid2motr, eid, &id);

	bsize = m0store_get_bsize(id);
	if (bsize < 0)
		return (int)bsize;

	written_bytes = m0store_pwrite(id, offset, buffer_size,
				       bsize, buffer);
	if (written_bytes < 0)
		return -1;

	RC_WRAP(update_stat, stat, UP_ST_WRITE,
		offset+written_bytes);

	snprintf(k, KLEN, "%.stat", eid->data);
	klen = strnlen(k, KLEN)+1;
	vlen = sizeof(struct stat);
	RC_WRAP(m0kvs_get, k, klen, (char *)&motr_stat, &vlen);
	RC_WRAP(update_stat, &motr_stat, UP_ST_WRITE,
		offset+written_bytes);
	
	klen = strnlen(k, KLEN)+1;
	vlen = sizeof(struct stat);
	RC_WRAP(m0kvs_set, k, klen, (char *)&motr_stat, vlen);

	*fsal_stable = true;
	return written_bytes;
}


int extstore_truncate(extstore_id_t *eid,
		      off_t filesize,
		      bool on_obj_store,
		      struct stat *stat)
{
	if (!eid || !stat)
		return -EINVAL;

	stat->st_size = filesize;

	return 0;
}

int extstore_getattr(extstore_id_t *eid,
		     struct stat *stat)
{
	return -ENOENT; /* Attrs are not ,anaged here */
}

int extstore_archive(extstore_id_t *eid)
{
	return -ENOTSUP;
}

int extstore_restore(extstore_id_t *eid)
{
	return -ENOTSUP;
}

int extstore_release(extstore_id_t *eid)
{
	return -ENOTSUP;
}

int extstore_state(extstore_id_t *eid,
		   char *state)
{
	return -ENOTSUP;
}

int extstore_cp_to(int fd_source,
		   extstore_id_t *eid,
		   int iolen,
		   size_t filesize)
{
	struct m0_uint128 id;
        size_t rsize, wsize;
        size_t bsize;
        char *buff;
        size_t remain;
        size_t aligned_offset;
        size_t nb;
        int rc;

	if (getenv("NO_BULK") != NULL) {
		fprintf(stderr, "====> NO BULK !!!\n");
		return -ENOTSUP;
	}

	RC_WRAP(eid2motr, eid, &id);

        bsize = m0store_get_bsize(id);
        if (bsize < 0)
                return bsize;

        remain = filesize % bsize;
        nb = filesize / bsize;
        aligned_offset = nb * bsize;

        printf("filesize=%zd bsize=%zd nb=%zd remain=%zd aligned_offset=%zd\n",
                filesize, bsize, nb, remain, aligned_offset);

        buff = malloc(bsize);
        if (buff == NULL)
                return -ENOMEM;

        rc = m0_write_bulk(fd_source,
                           id,
                           bsize,
                           nb,
                           0,
                           0);
        printf("===> rc=%d\n", rc);

        rsize = pread(fd_source, buff, remain, aligned_offset);
        if (rsize < 0) {
                        free(buff);
                        return -1;
        }

        wsize = m0store_pwrite(id, aligned_offset, rsize, bsize, buff);
        if (wsize < 0) {
                free(buff);
                return -1;
        }

        if (wsize != rsize) {
                free(buff);
                return -1;
        }

        /* Think about setting MD */
        free(buff);
        return 0;
}

int extstore_cp_from(int fd_dest,
		     extstore_id_t *eid,
		     int iolen,
		     size_t filesize)
{
	struct m0_uint128 id;
        size_t rsize, wsize;
        ssize_t bsize;
        char *buff;

        size_t remain;
        size_t aligned_offset;
        size_t nb;

        int rc;

	if (getenv("NO_BULK") != NULL) {
		fprintf(stderr, "====> NO BULK !!!\n");
		return -ENOTSUP;
	}

	RC_WRAP(eid2motr, eid, &id);

        bsize = m0store_get_bsize(id);
        if (bsize < 0) 
                return bsize;

        remain = filesize % bsize;
        nb = filesize / bsize;
        aligned_offset = nb * bsize;

        printf("filesize=%zd bsize=%zd nb=%zd remain=%zd aligned_offset=%zd\n",
                filesize, bsize, nb, remain, aligned_offset);

        buff = malloc(bsize);
        if (buff == NULL)
                return -ENOMEM;

        rc = m0_read_bulk(fd_dest,
                          id,
                          bsize,
                          nb,
                          0,
                          0,
                          0);
        printf("===> rc=%d\n", rc);

        rsize = m0store_pread(id, aligned_offset, remain, bsize, buff);
        if (rsize < 0) {
                free(buff);
                return -1;
        }

        wsize = pwrite(fd_dest, buff, rsize, aligned_offset);
        if (wsize < 0) {
                free(buff);
                return -1;
        }

        /* This POC writes on aligned blocks, we should align to real size */
        /* Useful in this case ?? */
        rc = ftruncate(fd_dest, filesize);
        if (rc < 0) {
                free(buff);
                return -1;
        }

        free(buff);
        return 0;
}

