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

/* kvsal_mero.c
 * KVS Abstraction Layer: interface for MERO 
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "kvsal_internal.h"
#include "../kvsal.h"

/* The REDIS context exists in the TLS, for MT-Safety */

struct m0_clovis_idx idx;

int kvsal_init(void)
{
	init_clovis();

	get_idx(&idx);
	return 0;
}

int kvsal_begin_transaction(void)
{
	return 0;
}

int kvsal_end_transaction(void)
{
	return 0;
}

int kvsal_discard_transaction(void)
{
	return 0;
}

int kvsal_exists(char *k)
{
	return 0;
}

int kvsal_set_char(char *k, char *v)
{
	return 0;
}

int kvsal_get_char(char *k, char *v)
{
	return 0;
}

int kvsal_set_stat(char *k, struct stat *buf)
{
	return 0;
}

int kvsal_get_stat(char *k, struct stat *buf)
{
	return 0;
}

int kvsal_set_binary(char *k, char *buf, size_t size)
{
	return 0;
}

int kvsal_get_binary(char *k, char *buf, size_t *size)
{
	return 0;
}

int kvsal_incr_counter(char *k, unsigned long long *v)
{
	return 0;
}

int kvsal_del(char *k)
{
	return 0;
}

int kvsal_get_list(char *pattern, int start, int *size, kvsal_item_t *items)
{
	return 0;
}

int kvsal_get_list_size(char *pattern)
{
	return 0;
}
