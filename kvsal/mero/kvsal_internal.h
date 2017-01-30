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

#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"

int init_clovis(void); 
int fini_clovis(void); 
void get_idx(struct m0_clovis_idx *idx);


