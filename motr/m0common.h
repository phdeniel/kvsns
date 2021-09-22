#ifndef _M0COMMON_H
#define _M0COMMON_H

/* -*- C -*- */
/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 *       * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <fnmatch.h>

#include "motr/client.h"
#include "motr/client_internal.h"
#include "motr/init.h"
#include "motr/idx.h"
#include "lib/memory.h"
#include "lib/thread.h"
#include "lib/user_space/trace.h"
#include <kvsns/kvsal.h>


/** Max number of blocks in concurrent IO per thread.
 *   * Currently Client can write at max 100 blocks in
 *     * a single request. This will change in future.
 *       */
enum { M0_MAX_BLOCK_COUNT = 100 };

enum {
	/** Min block size */
	BLK_SIZE_4k = 4096,
	/** Max block size */
	BLK_SIZE_32m = 32 * 1024 * 1024
};


typedef bool (*get_list_cb)(char *k, void *arg);

int m0init(struct collection_item *cfg_items);
void m0fini(void);
int m0_obj_id_sscanf(char *idstr, struct m0_uint128 *obj_id);
int m0kvs_reinit(void);

int m0kvs_set(char *k, size_t klen,
	       char *v, size_t vlen);
int m0kvs_get(char *k, size_t klen,
	       char *v, size_t *vlen);
int m0kvs_del(char *k, size_t klen);
void m0_iter_kvs(char *k);
int m0_pattern_kvs(char *k, char *pattern,
		   get_list_cb cb, void *arg_cb);

int m0store_create_object(struct m0_uint128 id);
int m0store_delete_object(struct m0_uint128 id);

#define M0STORE_BLK_COUNT 10

enum io_type {
	IO_READ = 1,
	IO_WRITE = 2
};

#define CONF_STR_LEN 100

ssize_t m0store_get_bsize(struct m0_uint128 id);

ssize_t m0store_do_io(struct m0_uint128 id, enum io_type iotype, off_t x,
		      size_t len, size_t bs, char *buff);

static inline ssize_t m0store_pwrite(struct m0_uint128 id, off_t x,
				     size_t len, size_t bs, char *buff)
{
	return m0store_do_io(id, IO_WRITE, x, len, bs, buff);
}

static inline ssize_t m0store_pread(struct m0_uint128 id, off_t x,
				    size_t len, size_t bs, char *buff)
{
	return m0store_do_io(id, IO_READ, x, len, bs, buff);
}

/*
 * Stuff for blk IO
 */

int m0_read(struct m0_container *container,
	    struct m0_uint128 id,
	    char *dest,
	    uint32_t block_size,
	    uint32_t block_count,
	    uint64_t offset,
	    int blks_per_io, bool take_locks,
	    uint32_t flags);


int m0_write_bulk(int fd_src,
		  struct m0_uint128 id,
		  uint32_t block_size,
		  uint32_t block_count,
		  uint64_t update_offset,
		  int blks_per_io);

int m0_read_bulk(int fd_dest,
		 struct m0_uint128 id,
		 uint32_t block_size,
		 uint32_t block_count,
		 uint64_t update_offset,
		 int blks_per_io,
		 uint32_t flags);

#endif
/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
