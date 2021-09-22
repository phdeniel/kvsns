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

/* extstore.h
 * KVSNS/extstore: header file for external storage interface
 */

#ifndef _POSIX_STORE_H
#define _POSIX_STORE_H

#include <libgen.h>		/* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <ini_config.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

int extstore_init(struct collection_item *cfg_items,
		  struct kvsal_ops *kvsalops);
int extstore_create(kvsns_ino_t object);
int extstore_read(kvsns_ino_t *ino,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat);
int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat);
int extstore_del(kvsns_ino_t *ino);
int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      bool running_attach,
		      struct stat *stat);
int extstore_attach(kvsns_ino_t *ino,
		    char *objid, int objid_len);
int extstore_getattr(kvsns_ino_t *ino,
		     struct stat *stat);

/* Pseudo HSM */
int extstore_archive(kvsns_ino_t *ino);
int extstore_restore(kvsns_ino_t *ino);
int extstore_release(kvsns_ino_t *ino);
int extstore_state(kvsns_ino_t *ino, char *state);

/* Bulk CP */
int extstore_cp_to(int fd,
		   kvsns_ino_t *ino,
		   int iolen,
		   size_t filesize);
int extstore_cp_from(int fd,
		     kvsns_ino_t *ino,
		     int iolen,
		     size_t filesize);

struct extstore_ops {
	int (*init)(struct collection_item *cfg_items,
		    struct kvsal_ops *kvsalops);
	int (*create)(kvsns_ino_t object);
	int (*read)(kvsns_ino_t *ino,
		    off_t offset,
		    size_t buffer_size,
		    void *buffer,
		    bool *end_of_file,
		    struct stat *stat);
	int (*write)(kvsns_ino_t *ino,
		     off_t offset,
		     size_t buffer_size,
		     void *buffer,
		     bool *fsal_stable,
		     struct stat *stat);
	int (*del)(kvsns_ino_t *ino);
	int (*truncate)(kvsns_ino_t *ino,
			off_t filesize,
			bool running_attach,
			struct stat *stat);
	int (*attach)(kvsns_ino_t *ino,
		      char *objid,
		      int objid_len);
	int (*getattr)(kvsns_ino_t *ino,
		       struct stat *stat);
	int (*archive)(kvsns_ino_t *ino);
	int (*restore)(kvsns_ino_t *ino);
	int (*release)(kvsns_ino_t *ino);
	int (*state)(kvsns_ino_t *ino,
		     char *state);
	int (*cp_to)(int fd,
		     kvsns_file_open_t *kfd,
		     int iolen,
		     size_t filesize);
	int (*cp_from)(int fd,
		       kvsns_file_open_t *kfd,
		       int iolen,
		       size_t filesize);
};

typedef int build_extstore_path_func(kvsns_ino_t object,
				     char *extstore_path,
				     size_t pathlen);

/* Objstore */
int objstore_init(struct collection_item *cfg_items,
		  struct kvsal_ops *kvsalops,
		  build_extstore_path_func *bespf);
int objstore_put(char *path, kvsns_ino_t *ino);
int objstore_get(char *path, kvsns_ino_t *ino);
int objstore_del(kvsns_ino_t *ino);

struct objstore_ops {
	int (*init)(struct collection_item *cfg_items,
		    struct kvsal_ops *kvsalops,
		    build_extstore_path_func *bespf);
	int (*put)(char *path, kvsns_ino_t *ino);
	int (*get)(char *path, kvsns_ino_t *ino);
	int (*del)(kvsns_ino_t *ino);
};

#endif
