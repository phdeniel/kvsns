#ifndef _POSIX_STORE_H
#define _POSIX_STORE_H

#include <libgen.h>		/* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include "../kvsns.h"

int external_init(char *rootpath);
int external_read(kvsns_ino_t *ino, 
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  size_t *read_amount,
		  bool *end_of_file);
int external_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   size_t *write_amount,
		   bool *fsal_stable,
		   struct stat *stat);
int external_consolidate_attrs(kvsns_ino_t *ino,
			       struct stat *stat);
#if 0
int external_unlink(struct fsal_obj_handle *dir_hdl,
		    const char *name);
#endif
int external_truncate(kvsns_ino_t *ino, off_t filesize);

#endif
