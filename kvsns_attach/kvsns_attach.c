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

/* kvsns_attach.c
 * KVSNS: attach existing objects to KVSNS
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h> /* for basename() */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h> /* For long options */
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

/* Flag set by ?--verbose?. */
static int verbose_flag;

static void help(char *exec)
{
	printf("%s  \t[-u <user>] [--user=<user>]	set user\n"
	       "\t	[-g <group>] [--group=<group>]	set group\n"
	      "\t	[-m <mode>] [--mode=<mode>]	set mode (octal)\n"
	      "\t	[-s <size>] [--size=<size>]	set size\n"
	      "\t	[-a <atime>] [--atime=<atime>]	set atime\n"
	      "\t	[-m <mtime>] [--mtime=<mtime>]	set mtime\n"
	      "\t	[-c <ctime>] [--ctime=<ctime>]	set ctime\n"
	      "\t	-p <path> --path=<path>		path in kvsns\n"
	      "\t	-o <objid> --objid=<objid>	objectid\n", exec);

	exit(0);
}

int main(int argc, char **argv)
{
	int c;

	while (1) {
		static struct option long_options[] = {
			/* These options set a flag. */
			{"verbose", no_argument,	   &verbose_flag, 1},
			{"brief",   no_argument,	   &verbose_flag, 0},
			{"help",   no_argument,	   &verbose_flag, 0},
			/* These options don?t set a flag.
			 * We distinguish them by their indices. */
			{"user",  required_argument, 0, 0},
			{"group",  required_argument, 0, 0},
			{"path",  required_argument, 0, 0},
			{"objid",  required_argument, 0, 0},
			{"mode",  required_argument, 0, 0},
			{"size",  required_argument, 0, 0},
			{"atime",  required_argument, 0, 0},
			{"mtime",  required_argument, 0, 0},
			{"ctime",  required_argument, 0, 0},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "vhu:g:p:o:s:m:a:c",
		long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			printf("option #%d %s", option_index,
				long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;

		case 'v':
			verbose_flag = 1;
			break;

		case 'u':
			printf("option -u with value `%s'\n", optarg);
			break;

		case 'g':
			printf("option -g with value `%s'\n", optarg);
			break;

		case 'p':
			printf("option -p with value `%s'\n", optarg);
			break;

		case 'o':
			printf("option -o with value `%s'\n", optarg);
			break;

		case 's':
			printf("option -s with value `%s'\n", optarg);
			break;

		case 'm':
			printf("option -m with value `%s'\n", optarg);
			break;

		case 'a':
			printf("option -a with value `%s'\n", optarg);
			break;

		case 'c':
			printf("option -c with value `%s'\n", optarg);
			break;

		case 'h':
		case '?':
		default:
			/* Display help */
			help(basename(argv[0]));
			break;
		}
	}

	/* Instead of reporting ?--verbose?
	 * and ?--brief? as they are encountered,
	 * we report the final status resulting from them. */
	if (verbose_flag)
		puts("verbose flag is set");

	/* Print any remaining command line arguments (not options). */
	if (optind < argc) {
		printf("Too many arguments provided !!\n");
		help(basename(argv[0]));
	}

	return 0;
}

