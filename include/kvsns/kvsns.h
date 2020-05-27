/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL  philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_test.c
 * KVSNS: simple test
 */

#ifndef _KVSNS_H
#define _KVSNS_H

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/xattr.h>

#include <kvsns/kvsal.h>

#define KVSNS_ROOT_INODE 2LL
#define KVSNS_ROOT_UID 0

#define KVSNS_DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"
#define KVSNS_STORE "KVSNS_STORE"
#define KVSNS_SERVER "KVSNS_SERVER"
#define KLEN 256
#define VLEN 256

#define KVSNS_URL "kvsns:"
#define KVSNS_URL_LEN 6

#define STAT_MODE_SET	0x001
#define STAT_UID_SET	0x002
#define STAT_GID_SET	0x004
#define STAT_SIZE_SET	0x008
#define STAT_ATIME_SET	0x010
#define STAT_MTIME_SET	0x020
#define STAT_CTIME_SET	0x040
#define STAT_INCR_LINK  0x080
#define STAT_DECR_LINK  0x100
#define STAT_SIZE_ATTACH  0x200

#define STAT_OWNER_READ		0400	/* Read by owner. */
#define STAT_OWNER_WRITE	0200	/* Write by owner. */
#define STAT_OWNER_EXEC		0100	/* Execute by owner. */
#define STAT_GROUP_READ		0040	/* Read by group. */
#define STAT_GROUP_WRITE	0020	/* Write by group. */
#define STAT_GROUP_EXEC		0010	/* Execute by group. */
#define STAT_OTHER_READ		0004	/* Read by other. */
#define STAT_OTHER_WRITE	0002	/* Write by other. */
#define STAT_OTHER_EXEC		0001	/* Execute by other. */

#define KVSNS_ACCESS_READ 	1
#define KVSNS_ACCESS_WRITE	2
#define KVSNS_ACCESS_EXEC	4

/* KVSAL related definitions and functions */


typedef unsigned long long int kvsns_ino_t;

/* KVSNS related definitions and functions */
typedef struct kvsns_cred__ {
	uid_t uid;
	gid_t gid;
} kvsns_cred_t;

typedef struct kvsns_fsstat_ {
	unsigned long nb_inodes;
} kvsns_fsstat_t;

typedef struct kvsns_dentry_ {
	char name[NAME_MAX];
	kvsns_ino_t inode;
	struct stat stats;
} kvsns_dentry_t;

typedef struct kvsns_open_owner_ {
	int pid;
	int tid;
} kvsns_open_owner_t;

typedef struct kvsns_file_open_ {
	kvsns_ino_t ino;
	kvsns_open_owner_t owner;
	int flags;
} kvsns_file_open_t;

typedef struct kvsns_dir {
	kvsns_ino_t ino;
	kvsal_list_t list;
} kvsns_dir_t;

enum kvsns_type {
	KVSNS_DIR = 1,
	KVSNS_FILE = 2,
	KVSNS_SYMLINK = 3
};

typedef struct kvsns_xattr__ {
	char name[NAME_MAX];
} kvsns_xattr_t;

/**
 * Start the kvsns library. This should be done by every thread using the library
 *
 * @note: this function will allocate required resources and set useful 
 * variables to their initial value. As the programs ends kvsns_stop() should be
 * invoked to perform all needed cleanups.
 * In this version of the API, it takes no parameter, but this may change in
 * later versions.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_start(const char *config);

/**
 * Stops the kvsns library. This should be done by every thread using the library
 *
 * @note: this function frees what kvsns_start() allocates.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_stop(void);


/**
 * Create the root of the namespace.
 *
 * @param openbar: if true, everyone can access the root directory
 * if false, only root can.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_init_root(int openbar);

/**
 * Check is a given user can access an inode 
 *
 * @note: this call is similar to POSIX's access() call. It behaves the same.
 *
 * @param cred - pointer to user's credentials
 * @param ino - pointer to inode whose access is to be checked.
 * @params flags - access to be tested. The flags are the same as those used
 * by libc's access() function.
 *
 * @return 0 if access is granted, a negative value means an error. -EPERM 
 * is returned when access is not granted
 */
int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags);

/**
 * Creates a file.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the file to be created
 * @param mode - Unix mode for the new entry
 * @paran newino - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_creat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newino);

/**
 * Creates a directory.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be created
 * @param mode - Unix mode for the new entry
 * @paran newino - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_mkdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newdir);

/**
 * Creates a symbolic link
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be created
 * @param content - the content of the symbolic link to be created
 * @paran newlnk - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_symlink(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 char *content, kvsns_ino_t *newlnk);

/**
 * Reads the content of a symbolic link
 *
 * @param cred - pointer to user's credentials
 * @param link - pointer to the symlink's inode
 * @param content - [OUT] buffer containing the read content.
 * @param size - [OUT] size read by this call
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_readlink(kvsns_cred_t *cred, kvsns_ino_t *link,
		 char *content, size_t *size);

/**
 * Removes a directory. It won't be deleted if not empty.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be remove.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name);


/**
 * Removes a file or a symbolic link.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the entry to be remove.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);

/**
 * Creates a file's hardlink
 *
 * @note: this call will failed if not performed on a file.
 *
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode.
 * @param dino - pointer to destination directory's inode
 * @param dname - name of the new entry in dino
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_link(kvsns_cred_t *cred, kvsns_ino_t *ino, kvsns_ino_t *dino,
	    char *dname);

/**
 * Renames an entry. 
 *
 * @param cred - pointer to user's credentials
 * @param sino - pointer to source directory's inode
 * @param sname - source name of the entry to be moved 
 * @param dino - pointer to destination directory's inode
 * @param dname - name of the new entry in dino
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_rename(kvsns_cred_t *cred, kvsns_ino_t *sino, char *sname,
		 kvsns_ino_t *dino, char *dname);

/**
 * Finds the inode of an entry whose parent and name are known. This is the
 * basic "lookup" operation every filesystem implements.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the entry to be found.
 * @paran myino - [OUT] points to the found ino if successful.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 kvsns_ino_t *myino);

/**
 * Finds the parent inode of an entry. 
 *
 * @note : because of this call, directories do not contain explicit
 * "." and ".." directories. The namespace's root directory is the only
 * directory to be its own parent directory.
 *
 * @param cred - pointer to user's credentials
 * @param where - pointer to current inode
 * @param name - name of the entry to be found.
 * @paran parent - [OUT] points to parent directory's inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *where, kvsns_ino_t *parent);

/** 
 * Finds the root of the namespace
 *
 * @param ino - [OUT] points to root inode if successful
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_get_root(kvsns_ino_t *ino);

/**
 * Gets attributes for a known inode.
 *
 * @note: the call is similar to stat() call in libc. It uses the structure
 * "struct stat" defined in the libC.
 *
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode
 * @param buffstat - [OUT] points to inode's stats.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *buffstat);

/**
 * Sets attributes for a known inode.
 *
 * This call uses a struct stat structure as input. This structure will
 * contain the values to be set. More than one can be set in a single call. 
 * The parameter "statflags: indicates which fields are to be considered:
 *  STAT_MODE_SET: sets mode
 *  STAT_UID_SET: sets owner
 *  STAT_GID_SET: set group owner
 *  STAT_SIZE_SET: set size (aka truncate)
 *  STAT_ATIME_SET: sets atime
 *  STAT_MTIME_SET: sets mtime
 *  STAT_CTIME_SET: set ctime
 *
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode
 * @param setstat - a stat structure containing the new values
 * @param statflags - a bitmap that tells which attributes are to be set 
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *setstat,
		 int statflags);

/**
 * Gets dynamic stats for the whole namespace
 *
 * @param stat - FS stats for the namespace
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_fsstat(kvsns_fsstat_t *stat);

/**
 * Open a directory to be accessed by kvsns_readdir
 *
 * @param cred - pointer to user's credentials
 * @param dir - pointer to directory's inode
 * @param ddir - [OUT] returned handle to opened directory
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_opendir(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_dir_t *ddir);

/**
 * Reads the content of a directory
 *
 * @param cred - pointer to user's credentials
 * @param dir - handle (return by kvsns_opendir) to the directory to be read
 * @param offset - offset position fo reading
 * @param dirent - [OUT] array of kvsns_dentry for reading
 * @param size - [INOUT] as input, allocated size of dirent arry, as output
 * read size.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_readdir(kvsns_cred_t *cred, kvsns_dir_t *dir, off_t offset,
		 kvsns_dentry_t *dirent, int *size);

/**
 * Closes a directory opened by kvsns_opendir
 *
 * @param ddir - handles to opened directory
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_closedir(kvsns_dir_t *ddir);

/**
 * Opens a file for reading and/or writing
 *
 * @note: this call use the same flags as LibC's open() call. You must know
 * the inode to call this function so you can't use it for creating a file.
 * In this case, kvsns_create() is to be invoked.
 *
 * @todo: mode parameter is unused. Remove it.
 *
 * @param cred - pointer to user's credentials
 * @param ino - file's inode
 * @param flags - open flags (see man 2 open)
 * @param mode - unused 
 * @param fd - [OUT] handle to opened file.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_open(kvsns_cred_t *cred, kvsns_ino_t *ino,
	    int flags, mode_t mode, kvsns_file_open_t *fd);

/**
 * Opens a file by name and parent directory
 *
 * @note: this call use the same flags as LibC's openat() call. You must know
 * the inode to call this function so you can't use it for creating a file.
 * In this case, kvsns_create() is to be invoked.
 *
 * @todo: mode parameter is unused. Remove it.
 *
 * @param cred - pointer to user's credentials
 * @param parent - parent's inode
 * @param name- file's name
 * @param flags - open flags (see man 2 open)
 * @param mode - unused 
 * @param fd - [OUT] handle to opened file.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_openat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 int flags, mode_t mode, kvsns_file_open_t *fd);

/**
 * Closes a file descriptor
 *
 * @param fd - handle to opened file
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_close(kvsns_file_open_t *fd);

/** 
 * Writes data to an opened fd
 *
 * @param cred - pointer to user's credentials
 * @param fd - handle to opened file
 * @param buf - data to be written
 * @param count - size of buffer to be written
 * @param offset - write offset
 *
 * @return written size or a negative "-errno" in case of failure
 */
ssize_t kvsns_write(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		  void *buf, size_t count, off_t offset);

/** 
 * Reads data from an opened fd
 *
 * @param cred - pointer to user's credentials
 * @param fd - handle to opened file
 * @param buf - [OUT] read data
 * @param count - size of buffer to be read
 * @param offset - read offset
 *
 * @return read size or a negative "-errno" in case of failure
 */
ssize_t kvsns_read(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		  void *buf, size_t count, off_t offset);

/* Xattr */

/**
 * Sets an xattr to an entry
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - xattr's name
 * @param value - buffer with xattr's value
 * @param size - size of previous buffer
 * @param flags - overwrite behavior, see "man 2 setxattr". Overwitte is
 * prohibited if set to XATTR_CREATE
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_setxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		  char *name, char *value, size_t size, int flags);

/**
 * Gets an xattr to an entry
 *
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - xattr's name
 * @param value - [OUT] buffer with xattr's value
 * @param size - [OUT] size of previous buffer
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		  char *name, char *value, size_t *size);

/**
 * Lists an entry's xattr
 *
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @paran offset - read offset
 * @param list - [OUT] list of found xattrs
 * @param size - [INOUT] In: size of list, Out: size of read list.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset,
		 kvsns_xattr_t *list, int *size);

/**
 * Removes an xattr
 *
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - name of xattr to be removed
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);

/**
 * Removes all xattr for an inode
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_remove_all_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino);

/* For utility */

/** 
 * Deletes everything in the namespace but the root directory...
 *
 * @param (node) - void function.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_mr_proper(void);


/**
 *  High level API: copy a file from the KVSNS to a POSIX fd
 *
 * @param cred - pointer to user's credentials
 * @param kfd  - pointer to kvsns's open fd
 * @param fd_dest - POSIX fd to copy file into
 * @param iolen -recommend IO size
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_cp_from(kvsns_cred_t *cred, kvsns_file_open_t *kfd,
		  int fd_dest, int iolen);

/**
 *  High level API: copy a file to the KVSNS from a POSIX fd
 *
 * @param cred - pointer to user's credentials
 * @param fd_dest - POSIX fd to retrieve data from
 * @param kfd  - pointer to kvsns's open fd
 * @param iolen -recommend IO size
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_cp_to(kvsns_cred_t *cred, int fd_source,
		kvsns_file_open_t *kfd, int iolen);

/**
 *  High level API: do a "lookup by path" operation
 *
 * @param cred - pointer to user's credentials
 * @param parent - root of the lookup operation
 * @param path - path inside the kvsns, starting at inode parent
 * @param ino - found inode if lookup is successful
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_lookup_path(kvsns_cred_t *cred, kvsns_ino_t *parent, char *path,
		      kvsns_ino_t *ino);


/**
 *  High level API: Attach an existing object to the KVSNS namespace
 *
 * @param cred - pointer to user's credentials
 * @param parent - directory where object is to be inserted
 * @param name - name of the entry to be created in parent directory
 * @param objid - a buffer that contains the objectid
 * @param objid_len - size in bytes of objid
 * @param stat - wanted stat for the entry
 * @param statflags - attrs to be set in new entry
 * @param newfile - inode for the newly created file
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_attach(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 char *objid, int objid_len, struct stat *stat,
		 int statflags, kvsns_ino_t *newfile);


/* Pseudo HSM */

/**
 *  High level API: do an 'archive' coperation (if supported by extstore)
 *
 * @param cred - pointer to user's credentials
 * @param ino - found inode if lookup is successful
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_archive(kvsns_cred_t *cred, kvsns_ino_t *ino);

/**
 *  High level API: do a 'release' coperation (if supported by extstore)
 *
 * @param cred - pointer to user's credentials
 * @param ino - found inode if lookup is successful
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_release(kvsns_cred_t *cred, kvsns_ino_t *ino);

/**
 *  High level API: do a 'hsm state' query (if supported by extstore)
 *
 * @param cred - pointer to user's credentials
 * @param ino - found inode if lookup is successful
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_state(kvsns_cred_t *cred, kvsns_ino_t *ino, char *state);

#endif
