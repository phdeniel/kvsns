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

/* kvsns_hsm.c
 * KVSNS: manage the HSM status from the command line
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h> /* for basename() and dirname() */
#include <stdlib.h>
#include <getopt.h> /* For long options */
#include <string.h>
#include <ctype.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

enum action {
	INVALID = -1,
	STATE  =  0,
	ARCHIVE =  1,
	RELEASE =  2,
	RESTORE =  3,
	LAST    = 4,
};

static const char * const action_names[] = {
	[STATE]  = "status",
	[ARCHIVE] = "archive",
	[RELEASE] = "release",
	[RESTORE] = "restore",
};

static inline void str_upper2lower(char *str)
{
	int i = 0;

	for (i = 0; str[i] != '\0' ; i++)
		str[i] = tolower(str[i]);
}

static enum action str2action(char *str)
{
	enum action res;

	str_upper2lower(str);

	for (res = STATE; res < LAST; res++)
		if (!strncmp(str, action_names[res], MAXNAMLEN))
			return res;

	/* Not found */
	return INVALID;
}

static void help(char *exec)
{
	fprintf(stderr, "%s: [status,migrate,release,restore] <path>\n", exec);
	exit(0);
}

static int do_action(enum action action, char *path)
{
	struct stat bufstat;
	int rc = 0;
	kvsns_cred_t cred;
	kvsns_ino_t ino;
	char state[VLEN];

	cred.uid = getuid();
	cred.gid = getgid();

	printf("Path: %s\n", path);

	rc = stat(path, &bufstat);
	if (rc == -1) {
		fprintf(stderr,
			"Can't stat %s: errno=%d|%s\n",
			path, errno, strerror(errno));
		return -1;
	}

	/* Apply action only on files */
	if (!S_ISREG(bufstat.st_mode)) {
		fprintf(stderr, "Error: %s is not a file\n", path);
		return -1;
	}

	/* Perform the ation */
	ino = bufstat.st_ino;

	switch (action) {
	case STATE:
		rc = kvsns_state(&cred, &ino, state);
		break;
	case ARCHIVE:
		rc = kvsns_archive(&cred, &ino);
		break;
	case RELEASE:
		rc = kvsns_release(&cred, &ino);
		break;
	case RESTORE:
		rc = kvsns_restore(&cred, &ino);
		break;
	default:
		fprintf(stderr,
			"BUG: invalid ation %d in do_action\n", action);
		exit(-1);
	}

	if (rc < 0)
		printf("Action on %s failed with status %d\n", path, rc);

	return 0;
}

int main(int argc, char **argv)
{
	char exec_name[MAXPATHLEN];
	char cmd[MAXNAMLEN];
	enum action action;
	int i = 0;
	int rc = 0;

	strncpy(exec_name, basename(argv[0]), MAXPATHLEN);

	if (argc < 3)
		help(exec_name); /* will exit */

	strncpy(cmd, argv[1], MAXNAMLEN);
	action = str2action(cmd);

	if (action == INVALID) {
		fprintf(stderr, "Invalid command %s\n", cmd);
		help(exec_name);
	}

	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_start: err=%d\n", rc);
		exit(1);
	}

	for (i = 2; i < argc ; i++)
		do_action(action, argv[i]);

	return 0;
}

