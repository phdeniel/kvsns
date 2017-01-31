/* -*- C -*- */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

int m0store_create_object(struct m0_uint128 id);
int m0_pread(struct m0_uint128 id, char *buff, int block_size, int block_count);
int m0_pwrite(struct m0_uint128 id, char *buff, int block_size, int block_count);

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
