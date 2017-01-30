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



/* Currently Clovis can write at max 200 blocks in
 *  * a single request. This will change in future. */
#define CLOVIS_MAX_BLOCK_COUNT (200)

/* Clovis parameters */
/* local_addr is the Clovis endpoint on Mero cluster */
static char *clovis_local_addr;

/* End point of the HA service of Mero */
static char *clovis_ha_addr;

/* End point of confd service of Mero */
static char *clovis_confd_addr;

/* Mero profile to be used */
static char *clovis_prof;


/* Index directory for KVS */
/* static char *clovis_index_dir = "/tmp/"; */

/* Clovis Instance */
static struct m0_clovis          *clovis_instance = NULL;

/* Clovis container */
static struct m0_clovis_container clovis_container;
static struct m0_clovis_realm     clovis_uber_realm;

/* Clovis Configuration */
static struct m0_clovis_config    clovis_conf;

/* static struct m0_clovis_idx idx;*/
extern struct m0_clovis_idx idx;

static void get_clovis_env(void)
{
	clovis_local_addr = getenv("CLOVIS_LOCAL_ADDR");
	assert(clovis_local_addr != NULL);

	clovis_ha_addr = getenv("CLOVIS_HA_ADDR");
	assert(clovis_ha_addr != NULL);

	clovis_confd_addr = getenv("CLOVIS_CONFD_ADDR");
	assert(clovis_confd_addr != NULL);

	clovis_confd_addr = getenv("CLOVIS_CONFD_ADDR");
	assert(clovis_confd_addr != NULL);

	clovis_prof = getenv("CLOVIS_PROFILE");
	assert(clovis_prof != NULL);
}


int init_clovis(void)
{
	int rc;

	get_clovis_env();

	/* Initialize Clovis configuration */
	clovis_conf.cc_is_oostore		= false;
	clovis_conf.cc_is_read_verify		= false;
	clovis_conf.cc_local_addr		= clovis_local_addr;
	clovis_conf.cc_ha_addr			= clovis_ha_addr;
	clovis_conf.cc_confd			= clovis_confd_addr;
	clovis_conf.cc_profile			= clovis_prof;
	clovis_conf.cc_tm_recv_queue_min_len	= M0_NET_TM_RECV_QUEUE_DEF_LEN;
	clovis_conf.cc_max_rpc_msg_size		= M0_RPC_DEF_MAX_RPC_MSG_SIZE;

#if 0
	/* Index service parameters */
	clovis_conf.cc_idx_service_id	= M0_CLOVIS_IDX_MOCK;
	clovis_conf.cc_idx_service_conf      = clovis_index_dir;
#endif
	clovis_conf.cc_idx_service_id	= M0_CLOVIS_IDX_MERO;
	clovis_conf.cc_idx_service_conf      = NULL;

	clovis_conf.cc_process_fid	   = "<0x7200000000000000:0>";

	/* Create Clovis instance */
	rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);
	if (rc != 0) {
		printf("Failed to initilise Clovis\n");
		goto err_exit;
	}

	/* Container is where Entities (object) resides.
 *	  * Currently, this feature is not implemented in Clovis.
 *		   * We have only single realm: UBER REALM. In future with multiple realms
 *			    * multiple applications can run in different containers. */
	m0_clovis_container_init(&clovis_container,
				 NULL, &M0_CLOVIS_UBER_REALM,
				 clovis_instance);

	rc = clovis_container.co_realm.re_entity.en_sm.sm_rc;
	if (rc != 0) {
		printf("Failed to open uber realm\n");
		goto err_exit;
	}

	clovis_uber_realm = clovis_container.co_realm;

	return 0;

err_exit:
        return rc;
}

void fini_clovis(void)
{
        m0_clovis_idx_fini(&idx);
        m0_clovis_fini(&clovis_instance, true);
}


void get_idx(struct m0_clovis_idx *idx)
{
        struct m0_fid ifid = M0_FID_TINIT('i', 8, 1);

         m0_clovis_idx_init(idx, &clovis_container.co_realm,
                                (struct m0_uint128 *)&ifid);
}


