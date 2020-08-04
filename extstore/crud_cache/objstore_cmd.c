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

#include <sys/time.h>   /* for gettimeofday */
#include <kvsns/extstore.h>


#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc != 0)	\
		return __rc; })

#define RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	if (__rc != 0)        \
		goto __label; })


/* The REDIS context exists in the TLS, for MT-Safety */
#define LEN_CMD (MAXPATHLEN+2*MAXNAMLEN)

static char cmd_put[LEN_CMD];
static char cmd_get[LEN_CMD];
static char cmd_del[LEN_CMD];

static struct collection_item *conf = NULL;
struct kvsal_ops kvsal;
struct objstore_ops objstore;

build_extstore_path_func *build_extstore_path;

int objstore_put(char *path, kvsns_ino_t *ino)
{
	char k[KLEN];
	char storepath[MAXPATHLEN];
	char objpath[MAXPATHLEN];
	char cmd[3*MAXPATHLEN];
	FILE *fp;

	RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);
	sprintf(objpath, "%s.archive", storepath);
	sprintf(cmd, cmd_put, storepath, objpath);

	fp = popen(cmd, "r");
	pclose(fp);

	snprintf(k, KLEN, "%llu.data_obj", *ino);
	RC_WRAP(kvsal.set_char, k, objpath);

	return 0;
}

int objstore_get(char *path, kvsns_ino_t *ino)
{
	char k[KLEN];
	char storepath[MAXPATHLEN];
	char objpath[MAXPATHLEN];
	char cmd[3*MAXPATHLEN];
	FILE *fp;

	RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);
	snprintf(k, KLEN, "%llu.data_obj", *ino);
	RC_WRAP(kvsal.get_char, k, objpath);
	sprintf(cmd, cmd_get, storepath, objpath);

	fp = popen(cmd, "r");
	pclose(fp);

	return 0;
}

int objstore_del(kvsns_ino_t *ino)
{
	char k[KLEN];
	char storepath[MAXPATHLEN];
	char objpath[MAXPATHLEN];
	char cmd[3*MAXPATHLEN];
	FILE *fp;

	RC_WRAP(build_extstore_path, *ino, storepath, MAXPATHLEN);
	snprintf(k, KLEN, "%llu.data_obj", *ino);

	if (kvsal.exists(k) != -ENOENT) {
		RC_WRAP(kvsal.get_char, k, objpath);
		sprintf(cmd, cmd_del, objpath);

		fp = popen(cmd, "r");
		pclose(fp);

	}

	return 0;
}

int objstore_init(struct collection_item *cfg_items,
		  struct kvsal_ops *kvsalops,
		  build_extstore_path_func *bespf)
{
	struct collection_item *item;

	build_extstore_path = bespf;

	if (cfg_items != NULL)
		conf = cfg_items;

	memcpy(&kvsal, kvsalops, sizeof(struct kvsal_ops));

	/* Deal with store_root */
	item = NULL;
	RC_WRAP(get_config_item, "objstore_cmd", "command_put",
				 cfg_items, &item);
	if (item == NULL)
		return -EINVAL;
	else
		strncpy(cmd_put, get_string_config_value(item, NULL),
			LEN_CMD);

	item = NULL;
	RC_WRAP(get_config_item, "objstore_cmd", "command_get",
				 cfg_items, &item);
	if (item == NULL)
		return -EINVAL;
	else
		strncpy(cmd_get, get_string_config_value(item, NULL),
			LEN_CMD);

	item = NULL;
	RC_WRAP(get_config_item, "objstore_cmd", "command_del",
				 cfg_items, &item);
	if (item == NULL)
		return -EINVAL;
	else
		strncpy(cmd_del, get_string_config_value(item, NULL),
			LEN_CMD);

	return 0;
}

