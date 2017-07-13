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

/* kvsns_handle.c
 * KVSNS: functions to manage handles
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <ini_config.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include <kvsns/extstore.h>
#include "kvsns_internal.h"

static char kvsns_store_default[] = KVSNS_STORE_DEFAULT;
static char kvsns_store_base[MAXPATHLEN];
extern kvsns_debug;
static struct collection_item *cfg_items;

static int kvsns_set_store_url(struct collection_item *cfg_items)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	char *store = NULL;
	struct collection_item *item;

	if (cfg_items == NULL)
		return -EINVAL;

	rc = get_config_item("basic", "store_url", cfg_items, &item);
	if (rc)
		return -rc;
	if (item == NULL)
		return -EINVAL;

	store = get_string_config_value(item, NULL);

	snprintf(k, KLEN, "store_url");
	if (store == NULL) {
		rc = kvsal_get_char(k, v);
		if (rc == 0)
			store = v;
		else
			store = kvsns_store_default;
	}
	RC_WRAP(kvsal_set_char, k, store);

	snprintf(kvsns_store_base, MAXPATHLEN, "%s", store);

	return 0;
}

void kvsns_set_debug(bool debug)
{
	kvsns_debug = debug;
}

int kvsns_start(const char *configpath)
{
	char k[KLEN];
	char v[VLEN];

	struct collection_item *errors = NULL;
	int rc;

	if (kvsns_debug)
		fprintf(stderr, "kvsns_start()\n");

	rc = config_from_file("libkvsns", configpath, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		if (kvsns_debug)
			fprintf(stderr, "Can't load config rc=%d\n", rc);

		free_ini_config_errors(errors);
		return -rc;
	}

	RC_WRAP(kvsal_init, cfg_items);

	RC_WRAP(kvsns_set_store_url, cfg_items);

	snprintf(k, KLEN, "store_url");
	RC_WRAP(kvsal_get_char, k, v);

	RC_WRAP(extstore_init, cfg_items);

	/** @todo : remove all existing opened FD (crash recovery) */
	return 0;
}

int kvsns_stop(void)
{
	RC_WRAP(kvsal_fini);
	free_ini_config_errors(cfg_items);
	return 0;
}

int kvsns_init_root(int openbar)
{
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;
	kvsns_cred_t cred;
	kvsns_ino_t ino;

	if (kvsns_debug)
		fprintf(stderr, "kvsns_init_root(%d)\n",
			openbar);

	cred.uid = 0;
	cred.gid = 0;
	ino = KVSNS_ROOT_INODE;

	kvsns_set_store_url(cfg_items);

	snprintf(k, KLEN, "%llu.parentdir", ino);
	snprintf(v, VLEN, "%llu|", ino);
	RC_WRAP(kvsal_set_char, k, v);

	snprintf(k, KLEN, "ino_counter");
	snprintf(v, VLEN, "3");
	RC_WRAP(kvsal_set_char, k, v);

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	if (openbar != 0)
		bufstat.st_mode = S_IFDIR|0777;
	else
		bufstat.st_mode = S_IFDIR|0755;
	bufstat.st_ino = KVSNS_ROOT_INODE;
	bufstat.st_nlink = 2;
	bufstat.st_uid = 0;
	bufstat.st_gid = 0;
	bufstat.st_atim.tv_sec = 0;
	bufstat.st_mtim.tv_sec = 0;
	bufstat.st_ctim.tv_sec = 0;

	snprintf(k, KLEN, "%llu.stat", ino);
	RC_WRAP(kvsal_set_stat, k, &bufstat);

	return 0;
}
