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


int extstore_init(char *rootpath)
{
	return 0;
}

int extstore_create(kvsns_ino_t object)
{
	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	return 0;
}

int extstore_read(kvsns_ino_t *ino,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	return 0;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	return 0;
}


int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      struct stat *stat)
{
	return 0;
}

