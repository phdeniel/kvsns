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
#include <dlfcn.h>  /* for dlopen and dlsym */

char funcname[MAXNAMLEN];

void *handle_objstore_lib;
struct objstore_ops objstore_lib_ops;

/* This macro is used to add a function through dlsym to a holding struct.
 * For instance, we call this macro with the module "objstore_cmd", the name
 * of the targeted function ("get"), the library handle ("libobjstore_cmd")
 * and the holding structure "objstore_lib_ops". This way, the "get" function
 * will correspond to the function "objstore_cmd_get" in the "libobjstore_cmd"
 * library.
 */
#define ADD_FUNC(__module__, __name__, __handle__, __holder__)       ({ \
	snprintf(funcname, MAXNAMLEN, "%s_%s", __module__, #__name__);  \
	*(void **)(&__holder__.__name__) = dlsym(__handle__, funcname); \
	if (!__holder__.__name__)				       \
		return -EINVAL; })

#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc != 0)	\
		return __rc; })

#define RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	if (__rc != 0)	\
		goto __label; })


/* The REDIS context exists in the TLS, for MT-Safety */
#define LEN_CMD (MAXPATHLEN+2*MAXNAMLEN)

static char store_root[MAXPATHLEN];

static struct collection_item *conf = NULL;
struct kvsal_ops kvsal;

enum state {
	INVAL      = -1,
	CACHED     = 0,	/* Cache is newer than  Object Store  */
	DUPLICATED = 1,	/* Cache is the same as Object Store  */
	RELEASED   = 2,	/* In cache, not in Object Store      */
	STATE_LAST = 3,	/* For iteration only */
};

static const char * const state_names[] = {
	[CACHED] = "CACHED",
	[DUPLICATED]  = "DUPLICATED",
	[RELEASED] = "RELEASED",
};

static const char *state2str(enum state state)
{
	if (state >= STATE_LAST || state < 0)
		return NULL;
	return state_names[state];
}

static enum state str2state(const char *str)
{
	int i;

	for (i = 0; i < STATE_LAST; i++)
		if (!strcmp(str, state_names[i]))
			return i;
	return INVAL;
}

static int get_entry_state(extstore_id_t *eid, enum state *state)
{
	char k[KLEN];
	char v[VLEN];

	if (!eid || !state)
		return -EINVAL;

	snprintf(k, KLEN, "%s.cache_state", eid->data);
	RC_WRAP(kvsal.get_char, k, v);

	*state = str2state(v);

	return 0;
}

static int set_entry_state(extstore_id_t *eid, enum state state)
{
	char  k[KLEN];
	char *v = NULL;

	if (!eid || !state)
		return -EINVAL;

	snprintf(k, KLEN, "%s.cache_state", eid->data);
	v = (char *)state2str(state);

	RC_WRAP(kvsal.set_char, k, v);

	return 0;
}

static int del_entry_state(extstore_id_t *eid)
{
	char k[KLEN];

	if (!eid)
		return -EINVAL;

	snprintf(k, KLEN, "%s.cache_state", eid->data);
	RC_WRAP(kvsal.del, k);

	return 0;
}

static int build_extstore_path(extstore_id_t eid,
			       char *extstore_path,
			       size_t pathlen)
{
	if (!extstore_path)
		return -1;

	snprintf(extstore_path, pathlen, "%s/%s",
		 store_root, eid.data);

	return 0;
}

enum update_stat_how {
	UP_ST_WRITE = 1,
	UP_ST_READ = 2,
	UP_ST_TRUNCATE = 3
};

static int update_stat(struct stat *stat, enum update_stat_how how,
		       off_t size)
{
	struct timeval t;

	if (!stat)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	switch (how) {
	case UP_ST_WRITE:
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
		stat->st_ctim = stat->st_mtim;
		if (size > stat->st_size) {
			stat->st_size = size;
			stat->st_blocks = size / DEV_BSIZE;
		}
		break;

	case UP_ST_READ:
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
		break;

	case UP_ST_TRUNCATE:
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
		stat->st_mtim = stat->st_ctim;
		stat->st_size = size;
		stat->st_blocks = size / DEV_BSIZE;
		break;

	default: /* Should not occur */
		return -EINVAL;
		break;
	}

	return 0;
}

int extstore_new_objectid(extstore_id_t *eid,
			  unsigned int seedlen,
			  char *seed)
{
	if (!eid || !seed)
		return -EINVAL;

	/* Be careful about printf format: string with known length */
	eid->len = snprintf(eid->data, KLEN, "obj:%.*s", seedlen, seed);

	return 0;
}

int extstore_create(extstore_id_t eid)
{
	char path[sizeof(store_root) + KLEN];
	char v[sizeof(store_root) + KLEN];
	struct stat stat;
	char k[KLEN];
	int fd;

	RC_WRAP(build_extstore_path, eid, path, sizeof(store_root) + KLEN);

	fd = creat(path, 0777);
	if (fd == -1)
		return -errno;
	close(fd);

	snprintf(k, KLEN, "%s.data_attr", eid.data);
	memset((char *)&stat, 0, sizeof(stat));
	RC_WRAP(kvsal.set_stat, k, &stat);

	snprintf(k, KLEN, "%s.cache_state", eid.data);
	snprintf(v, VLEN, state2str(CACHED));
	RC_WRAP(kvsal.set_char, k, v);

	return 0;
}

int extstore_attach(extstore_id_t *eid)
{
	char k[KLEN];
	char v[VLEN];
	struct stat stat;

	if (!eid)
		return -EINVAL;

	snprintf(k, KLEN, "%s.data_attr", eid->data);
	memset((char *)&stat, 0, sizeof(stat));
	RC_WRAP(kvsal.set_stat, k, &stat);

	snprintf(k, KLEN, "%s.cache_state", eid->data);
	snprintf(v, VLEN, state2str(CACHED));
	RC_WRAP(kvsal.set_char, k, v);

	RC_WRAP(set_entry_state, eid, CACHED);

	return 0;
}

static int load_objstore_lib(struct collection_item *cfg_items,
			     struct kvsal_ops *kvsalops,
			     const char *lib_name, const char *module_name)
{
	struct collection_item *item = NULL;
	char *objstore_lib_path = NULL;

	RC_WRAP(get_config_item, "crud_cache", lib_name, cfg_items, &item);
	if (item == NULL)
		return -ENOENT;
	else
		objstore_lib_path = get_string_config_value(item, NULL);

	handle_objstore_lib = dlopen(objstore_lib_path, RTLD_LAZY);
	if (!handle_objstore_lib) {
		fprintf(stderr, "Can't open %s\n", objstore_lib_path);
		return -EINVAL;
	}

	ADD_FUNC(module_name, init, handle_objstore_lib, objstore_lib_ops);
	ADD_FUNC(module_name, get, handle_objstore_lib, objstore_lib_ops);
	ADD_FUNC(module_name, put, handle_objstore_lib, objstore_lib_ops);
	ADD_FUNC(module_name, del, handle_objstore_lib, objstore_lib_ops);

	RC_WRAP(objstore_lib_ops.init, cfg_items, kvsalops,
		&build_extstore_path);

	return 0;
}

int extstore_init(struct collection_item *cfg_items,
		  struct kvsal_ops *kvsalops)
{
	struct collection_item *item;
	int rc;

	if (cfg_items != NULL)
		conf = cfg_items;

	memcpy(&kvsal, kvsalops, sizeof(struct kvsal_ops));

	/* Deal with store_root */
	item = NULL;
	RC_WRAP(get_config_item, "crud_cache", "root_path", cfg_items, &item);
	if (item == NULL)
		return -EINVAL;
	else
		strncpy(store_root, get_string_config_value(item, NULL),
			MAXPATHLEN);

	/* Try to load the objstore_lib */
	rc = load_objstore_lib(cfg_items, kvsalops, "objstore_lib", "objstore");

	return rc;
}

int extstore_del(extstore_id_t *eid)
{
	char k[KLEN];
	char storepath[MAXPATHLEN];
	int rc;

	if (!eid)
		return -EINVAL;

	/* Delete in the object store */
	RC_WRAP(objstore_lib_ops.del, eid);

	rc = build_extstore_path(*eid, storepath, MAXPATHLEN);
	if (rc) {
		if (rc == -ENOENT) /* No data created */
			return 0;

		return rc;
	}

	rc = unlink(storepath);
	if (rc) {
		if (errno == ENOENT)
			return 0;

		return -errno;
	}

	/* delete <eid>.data_attr */
	snprintf(k, KLEN, "%s.data_attr", eid->data);
	RC_WRAP(kvsal.del, k);

	snprintf(k, KLEN, "%s.data_obj", eid->data);
	if (kvsal.exists(k) != -ENOENT)
		RC_WRAP(kvsal.del, k);

	/* delete state */
	RC_WRAP(del_entry_state, eid);

	return 0;
}

int extstore_read(extstore_id_t *eid,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc = 0;
	int fd = 0;
	ssize_t read_bytes;
	enum state state;
	char k[KLEN];

	if (!eid)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);

	if (state == RELEASED)
		RC_WRAP(extstore_restore, eid);

	RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);

	fd = open(storepath, O_CREAT|O_RDONLY|O_SYNC, 0755);
	if (fd < 0)
		return -errno;

	read_bytes = pread(fd, buffer, buffer_size, offset);
	if (read_bytes < 0) {
		close(fd);
		return -errno;
	}

	RC_WRAP_LABEL(rc, errout, update_stat, stat, UP_ST_READ, 0);
	snprintf(k, KLEN, "%s.data_attr", eid->data);
	RC_WRAP_LABEL(rc, errout, kvsal.set_stat, k, stat);

	rc = close(fd);
	if (rc < 0)
		return -errno;

	return read_bytes;

errout:
	close(fd);

	return rc;
}

int extstore_write(extstore_id_t *eid,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc;
	int fd;
	ssize_t written_bytes;
	struct stat objstat;
	enum state state;
	char k[KLEN];

	if (!eid)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);

	if (state == RELEASED)
		RC_WRAP(extstore_restore, eid);

	RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);

	fd = open(storepath, O_CREAT|O_WRONLY|O_SYNC, 0755);
	if (fd < 0)
		return -errno;

	written_bytes = pwrite(fd, buffer, buffer_size, offset);
	if (written_bytes < 0) {
		close(fd);
		return -errno;
	}

	rc = close(fd);
	if (rc < 0)
		return -errno;

	snprintf(k, KLEN, "%s.data_attr", eid->data);
	RC_WRAP(kvsal.get_stat, k, &objstat);
	RC_WRAP(update_stat, &objstat, UP_ST_WRITE,
		offset+written_bytes);
	RC_WRAP(kvsal.set_stat, k, &objstat);

	stat->st_size = objstat.st_size;
	stat->st_blocks = objstat.st_blocks;
	stat->st_mtim = objstat.st_mtim;
	stat->st_ctim = objstat.st_ctim;

	if (state != CACHED)
		RC_WRAP(set_entry_state, eid, CACHED);

	*fsal_stable = true;
	return written_bytes;
}


int extstore_truncate(extstore_id_t *eid,
		      off_t filesize,
		      bool running_attach,
		      struct stat *stat)
{
	int rc;
	char storepath[MAXPATHLEN];
	struct stat objstat;
	enum state state;
	char k[KLEN];

	if (!eid || !stat)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);


	switch (state) {
	case CACHED:
		RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);
		RC_WRAP(truncate, storepath, filesize);
		rc = 0;
		break;
	case DUPLICATED:
		RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);
		RC_WRAP(truncate, storepath, filesize);
		RC_WRAP(set_entry_state, eid, CACHED);
		rc = 0;
		break;
	case RELEASED:
		snprintf(k, KLEN, "%s.data_attr", eid->data);
		RC_WRAP(kvsal.get_stat, k, &objstat);

		stat->st_size = filesize;
		objstat.st_size = filesize;
		RC_WRAP(update_stat, &objstat, UP_ST_TRUNCATE, filesize);
		stat->st_ctim = objstat.st_ctim;
		stat->st_mtim = objstat.st_mtim;

		RC_WRAP(kvsal.set_stat, k, &objstat);
		rc = 0;
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_getattr(extstore_id_t *eid,
		     struct stat *stat)
{
	int rc;
	char storepath[MAXPATHLEN];
	enum state state;
	char k[KLEN];

	/* Should be using stored stat */
	if (!eid || !stat)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);

	switch (state) {
	case CACHED:
	case DUPLICATED:
		/* Newer data are in cache, let's query it */
		RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);
		RC_WRAP(lstat, storepath, stat);
		rc = 0; /* Success */
		break;
	case RELEASED:
		/* Get archived stats */
		snprintf(k, KLEN, "%s.data_attr", eid->data);
		RC_WRAP(kvsal.get_stat, k, stat);
		rc = 0; /* Success */
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_archive(extstore_id_t *eid)
{
	enum state state;
	char storepath[MAXPATHLEN];
	int rc;

	if (!eid)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);

	switch (state) {
	case RELEASED:
		rc = -EACCES;
		break;
	case DUPLICATED:
		rc = 0;
		break;
	case CACHED:
		/* Xfer wuth the object store */
		RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);

		RC_WRAP(objstore_lib_ops.put, storepath, eid);
		RC_WRAP(set_entry_state, eid, DUPLICATED);
		rc = 0;
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_restore(extstore_id_t *eid)
{
	enum state state;
	char storepath[MAXPATHLEN];
	int rc = 0;

	if (!eid)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);

	switch (state) {
	case DUPLICATED:
	case CACHED:
		rc = 0;
		break;
	case RELEASED:
		/* Xfer with the object store */
		RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);
		RC_WRAP(objstore_lib_ops.get, storepath, eid);
		RC_WRAP(set_entry_state, eid, DUPLICATED);
		rc = 0;
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_release(extstore_id_t *eid)
{
	enum state state;
	char storepath[MAXPATHLEN];
	int rc = 0;

	if (!eid)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);

	switch (state) {
	case CACHED:
		rc = -EACCES;
		break;
	case RELEASED:
		rc = 0; /* Nothing to do */
		break;
	case DUPLICATED:
		RC_WRAP(build_extstore_path, *eid, storepath, MAXPATHLEN);
		RC_WRAP(unlink, storepath);
		RC_WRAP(set_entry_state, eid, RELEASED);
		rc = 0;
		break;
	default:
		rc = -EINVAL; /* Should not occur */
		break;
	}

	return rc;
}

int extstore_state(extstore_id_t *eid, char *strstate)
{
	enum state state;

	if (!eid)
		return -EINVAL;

	RC_WRAP(get_entry_state, eid, &state);
	strcpy(strstate, state2str(state));

	return 0;
}

int extstore_cp_to(int fd,
		   extstore_id_t *eid, 
		   int iolen,
		   size_t filesize)
{
	return -ENOTSUP;
}

int extstore_cp_from(int fd,
		     extstore_id_t *eid, 
		     int iolen,
		     size_t filesize)
{
	return -ENOTSUP;
}

