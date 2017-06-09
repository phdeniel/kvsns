/* -*- C -*- */
#ifndef _M0_STORE_H
#define _M0_STORE_H
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

int m0store_create_object(struct m0_uint128 id);
int m0store_delete_object(struct m0_uint128 id);
int m0store_init(void);
void m0store_fini(void);

#define M0STORE_BLK_SIZE 4096
#define M0STORE_BLK_COUNT 10

enum io_type {
	IO_READ = 1,
	IO_WRITE = 2
};

#define CONF_STR_LEN 100
typedef struct m0store_config {
	char clovis_local_addr[CONF_STR_LEN];
	char clovis_ha_addr[CONF_STR_LEN];
	char clovis_confd_addr[CONF_STR_LEN];
	char clovis_prof[CONF_STR_LEN];
	char clovis_proc_fid[CONF_STR_LEN];
	char clovis_index_dir[CONF_STR_LEN];
} m0store_config_t;

ssize_t m0store_do_io(struct m0_uint128 id, enum io_type iotype, off_t x,
		      size_t len, size_t bs, char *buff);

static inline ssize_t m0store_pwrite(struct m0_uint128 id, off_t x,
				     size_t len, size_t bs, char *buff)
{
	return m0store_do_io(id, IO_WRITE, x, len, bs, buff);
}

static inline ssize_t m0store_pread(struct m0_uint128 id, off_t x,
				    size_t len, size_t bs, char *buff)
{
	return m0store_do_io(id, IO_READ, x, len, bs, buff);
}

#endif
/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
