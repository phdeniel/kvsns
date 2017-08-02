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

/* kvsns_cp.c
 * KVSNS: copies files to/from KVSNS
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include <kvsns/extstore.h>
#include "kvsns_internal.h"

#define BUFFSIZE 40960

int kvsns_cp_from(kvsns_cred_t *cred, kvsns_file_open_t *kfd,
		  int fd_dest, int iolen)
{
	off_t off;
	ssize_t rsize, wsize;
	int rc;
	int remains;
	size_t len;
	char buff[BUFFSIZE];
	struct stat stat;
	size_t filesize;

	rc = kvsns_getattr(cred, &kfd->ino, &stat);
	if (rc < 0)
		return rc;

	filesize = stat.st_size;
	remains = filesize;
	off = 0LL;
	while (off < filesize) {
		len = (remains > iolen) ? iolen : remains;

		rsize = kvsns_read(cred, kfd, buff, len, off);
		if (rsize < 0)
			return -1;

		wsize = pwrite(fd_dest, buff, rsize, off);
		if (wsize < 0)
			return -1;

		if (wsize != rsize)
			return -1;

		off += rsize;
		remains -= rsize;
	}

	/* This POC writes on aligned blocks, we should align to real size */
	/* Useful in this case ?? */
	rc = ftruncate(fd_dest, filesize);
	if (rc < 0)
		return -1;

	rc = fchmod(fd_dest, stat.st_mode);
	if (rc < 0)
		return -1;

	return 0;
}

int kvsns_cp_to(kvsns_cred_t *cred, int fd_source,
		kvsns_file_open_t *kfd, int iolen)
{
	off_t off;
	ssize_t rsize, wsize;
	int remains;
	size_t len;
	char buff[BUFFSIZE];
	int rc;
	struct stat srcstat;
	size_t filesize;

	rc = fstat(fd_source, &srcstat);
	if (rc < 0)
		return -errno;

	remains = srcstat.st_size;
	filesize = srcstat.st_size;
	off = 0LL;
	while (off < filesize) {
		len = (remains > iolen) ? iolen : remains;

		rsize = pread(fd_source, buff, len, off);
		if (rsize < 0)
			return -1;

		wsize = kvsns_write(cred, kfd, buff, rsize, off);
		if (wsize < 0)
			return -1;

		if (wsize != rsize)
			return -1;

		off += rsize;
		remains -= rsize;
	}

	rc = kvsns_setattr(cred, &kfd->ino, &srcstat, STAT_MODE_SET);
	if (rc < 0)
		return rc;

	return 0;
}

