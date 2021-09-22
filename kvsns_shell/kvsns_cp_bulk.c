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

#define IOLEN 4096

static void exit_rc(char *msg, int rc)
{
	if (rc >= 0)
		return;

	char format[MAXPATHLEN];

	snprintf(format, MAXPATHLEN, "%s%s",
		 msg, " |rc=%d\n");

	fprintf(stderr, format, rc);
	exit(1);
}

int main(int argc, char *argv[])
{
	kvsns_cred_t cred;
	kvsns_ino_t parent;
	kvsns_ino_t ino;
	kvsns_file_open_t kfd;
	int fd = 0;
	bool kvsns_src = false;
	bool kvsns_dest = false;
	char *src = NULL;
	char *dest = NULL;
	int rc;

	if (argc != 3) {
		fprintf(stderr, "%s <src> <dest>\n", argv[0]);
		exit(1);
	}

	cred.uid = getuid();
	cred.gid = getgid();

	if (!strncmp(argv[1], KVSNS_URL, KVSNS_URL_LEN)) {
		kvsns_src = true;
		src = argv[1] + KVSNS_URL_LEN;
		dest = argv[2];
	}

	if (!strncmp(argv[2], KVSNS_URL, KVSNS_URL_LEN)) {
		kvsns_dest = true;
		src = argv[1];
		dest = argv[2] + KVSNS_URL_LEN;
	}

	printf("%s => %s, %u / %u\n", src, dest, kvsns_src, kvsns_dest);

	if (kvsns_src && kvsns_dest) {
		fprintf(stderr, "src and destination can't be inside KVSNS\n");
		exit(1);
	}

	if (!kvsns_src && !kvsns_dest) {
		fprintf(stderr, "src or dest should be inside kvsns\n");
		exit(1);
	}

	if (!kvsns_dest)
		fd = open(dest, O_WRONLY|O_CREAT, 0644);

	if (!kvsns_src)
		fd = open(src, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr, "Can't open POSIX fd, errno=%u\n", errno);
		exit(1);
	}

	/* Start KVSNS Lib */
	rc = kvsns_start(NULL);
	exit_rc("kvsns_start faild", rc);

	rc = kvsns_get_root(&parent);
	exit_rc("Can't get KVSNS's root inode", rc);

	if (kvsns_src) {
		rc = kvsns_lookup_path(&cred, &parent, src, &ino);
		exit_rc("Can't lookup dest in KVSNS", rc);
		rc = kvsns_open(&cred, &ino, O_RDONLY, 0644, &kfd);
	}

	if (kvsns_dest) {
		rc = kvsns_lookup_path(&cred, &parent, dest, &ino);
		if (rc == -2) {
			rc = kvsns_creat(&cred, &parent, dest, 0644, &ino);
			exit_rc("Can't create dest file in KVSNS", rc);
		} else
			exit_rc("Can't lookup dest in KVSNS", rc);

		rc = kvsns_open(&cred, &ino, O_WRONLY, 0644, &kfd);
	}
	exit_rc("Can't open file in KVSNS", rc);

	/* Deal with the copy */
	if (kvsns_src)
		rc = kvsns_cp_from(&cred, &kfd, fd, IOLEN);

	if (kvsns_dest)
		rc = kvsns_cp_to(&cred, fd, &kfd, IOLEN);

	exit_rc("Copy failed", rc);

	/* The end */
	rc = close(fd);
	exit_rc("Can't close POSIX fd, errono", errno);

	rc = kvsns_close(&kfd);
	exit_rc("Can't close KVSNS fd, errono", errno);

	return 0;
}
