/* -*- C -*- */
/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 * 
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 *       * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <fnmatch.h>

#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"

#include "../kvsal.h"

extern struct m0_clovis_idx idx;

int init_clovis(void); 
void fini_clovis(void); 
void get_idx(struct m0_clovis_idx *idx);


int m0_get_kvs(struct m0_clovis_idx *idx, char *k, size_t klen,
	       char *v, size_t vlen);
int m0_set_kvs(struct m0_clovis_idx *idx, char *k, size_t klen,
	       char *v, size_t vlen);
int m0_del_kvs(struct m0_clovis_idx *idx, char *k);
void m0_list_kvs(struct m0_clovis_idx *idx);
void m0_iter_kvs(struct m0_clovis_idx *idx, char *k);
int m0_pattern_kvs(struct m0_clovis_idx *idx, char *k, char *pattern);
int m0_pattern_kvs_size(struct m0_clovis_idx *idx, char *k, char *pattern);

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
