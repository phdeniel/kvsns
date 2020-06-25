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
#include <dlfcn.h>  /* for dlopen and dlsym */
#include <syslog.h>
#include <ini_config.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include <kvsns/extstore.h>
#include "kvsns_internal.h"

static struct collection_item *cfg_items;

char funcname[MAXNAMLEN];

void *handle_extstore;
struct extstore_ops extstore;

void *handle_kvsal;
struct kvsal_ops kvsal;

#define LOG_DEFAULT LOG_CRIT

#define ADD_FUNC(__module__, __name__, __handle__)		     ({ \
	snprintf(funcname, MAXNAMLEN, "%s_%s", #__module__, #__name__); \
	*(void **)(&__module__.__name__) = dlsym(__handle__, funcname); \
	if (!__module__.__name__)				        \
		return -EINVAL; })

static int kvsns_load_libs(void)
{
	/* Read path to extstore lib in config file */
	struct collection_item *item = NULL;
	char *extstore_lib_path;
	char *kvsal_lib_path;

	/* Load kvsal library */
	RC_WRAP(get_config_item, "kvsns", "kvsal_lib", cfg_items, &item);
	if (item == NULL) {
		LogCrit("Can't find [kvsns]:kvsal_lib in config file");
		return -EINVAL;
	} else
		kvsal_lib_path = get_string_config_value(item, NULL);

	handle_kvsal = dlopen(kvsal_lib_path, RTLD_LAZY);
	if (!handle_kvsal) {
		LogCrit("Can't open %s errno=%d",
			kvsal_lib_path, errno);
		return -EINVAL;
	}

	ADD_FUNC(kvsal, init, handle_kvsal);
	ADD_FUNC(kvsal, fini, handle_kvsal);
	ADD_FUNC(kvsal, begin_transaction, handle_kvsal);
	ADD_FUNC(kvsal, end_transaction, handle_kvsal);
	ADD_FUNC(kvsal, exists, handle_kvsal);
	ADD_FUNC(kvsal, set_char, handle_kvsal);
	ADD_FUNC(kvsal, get_char, handle_kvsal);
	ADD_FUNC(kvsal, set_binary, handle_kvsal);
	ADD_FUNC(kvsal, get_binary, handle_kvsal);
	ADD_FUNC(kvsal, set_stat, handle_kvsal);
	ADD_FUNC(kvsal, get_stat, handle_kvsal);
	ADD_FUNC(kvsal, get_list_size, handle_kvsal);
	ADD_FUNC(kvsal, get_list, handle_kvsal);
	ADD_FUNC(kvsal, del, handle_kvsal);
	ADD_FUNC(kvsal, incr_counter, handle_kvsal);
	ADD_FUNC(kvsal, get_list_pattern, handle_kvsal);
	ADD_FUNC(kvsal, get_list, handle_kvsal);
	ADD_FUNC(kvsal, fetch_list, handle_kvsal);
	ADD_FUNC(kvsal, dispose_list, handle_kvsal);
	ADD_FUNC(kvsal, init_list, handle_kvsal);

	/* Load extstore library */
	RC_WRAP(get_config_item, "kvsns", "extstore_lib", cfg_items, &item);
	if (item == NULL) {
		LogCrit("Can't find [kvsns]:extstore_lib in config file");
		return -EINVAL;
	} else
		extstore_lib_path = get_string_config_value(item, NULL);

	handle_extstore = dlopen(extstore_lib_path, RTLD_LAZY);
	if (!handle_extstore) {
		LogCrit("Can't open %s errno=%d",
			extstore_lib_path, errno);
		return -EINVAL;
	}

	ADD_FUNC(extstore, init, handle_extstore);
	ADD_FUNC(extstore, create, handle_extstore);
	ADD_FUNC(extstore, read, handle_extstore);
	ADD_FUNC(extstore, write, handle_extstore);
	ADD_FUNC(extstore, del, handle_extstore);
	ADD_FUNC(extstore, truncate, handle_extstore);
	ADD_FUNC(extstore, attach, handle_extstore);
	ADD_FUNC(extstore, getattr, handle_extstore);
	ADD_FUNC(extstore, archive, handle_extstore);
	ADD_FUNC(extstore, restore, handle_extstore);
	ADD_FUNC(extstore, release, handle_extstore);
	ADD_FUNC(extstore, state, handle_extstore);

	return 0;
}

static int kvsns_init_log(void)
{
	struct collection_item *item = NULL;
	int level;
	char *strlevel = NULL;

	RC_WRAP(get_config_item, "kvsns", "log_level", cfg_items, &item);
	if (item == NULL) {
		/* Log level is not set, using default */
		level = LOG_DEFAULT;
	} else {
		strlevel = get_string_config_value(item, NULL);
		if (!strcmp(strlevel, "LOG_EMERG")) {
			level = LOG_EMERG;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_ALERT")) {
			level = LOG_ALERT;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_CRIT")) {
			level = LOG_CRIT;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_ERR")) {
			level = LOG_ERR;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_WARNING")) {
			level = LOG_WARNING;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_NOTICE")) {
			level = LOG_NOTICE;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_INFO")) {
			level = LOG_INFO;
			goto done;
		}
		if (!strcmp(strlevel, "LOG_DEBUG")) {
			level = LOG_DEBUG;
			goto done;
		}

		level = LOG_DEFAULT; /* default value */
	}

done:
	openlog("libkvsns", LOG_PID, LOG_DAEMON);
	setlogmask(LOG_UPTO(level));

	return 0;
}


int kvsns_start(const char *configpath)
{
	struct collection_item *errors = NULL;
	int rc;

	rc = config_from_file("libkvsns", configpath, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		fprintf(stderr, "Can't load config rc=%d\n", rc);

		free_ini_config_errors(errors);
		return -rc;
	}

	RC_WRAP(kvsns_init_log);

	RC_WRAP(kvsns_load_libs);

	RC_WRAP(kvsal.init, cfg_items);

	RC_WRAP(extstore.init, cfg_items, &kvsal);

	LogEvent("KVSNS is started");

	/** @todo : remove all existing opened FD (crash recovery) */
	return 0;
}

int kvsns_stop(void)
{
	RC_WRAP(kvsal.fini);
	LogEvent("KVSNS is stoping");
	free_ini_config_errors(cfg_items);
	closelog(); /* for syslog */
	return 0;
}

int kvsns_init_root(int openbar)
{
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;
	kvsns_ino_t ino;

	ino = KVSNS_ROOT_INODE;

	snprintf(k, KLEN, "%llu.parentdir", ino);
	snprintf(v, VLEN, "%llu|", ino);
	RC_WRAP(kvsal.set_char, k, v);

	snprintf(k, KLEN, "ino_counter");
	snprintf(v, VLEN, "3");
	RC_WRAP(kvsal.set_char, k, v);

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
	RC_WRAP(kvsal.set_stat, k, &bufstat);

	return 0;
}
