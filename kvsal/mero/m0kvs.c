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
#include "m0kvs.h"


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

/* Clovis Configuration */
static struct m0_clovis_config    clovis_conf;

/* static struct m0_clovis_idx idx;*/
extern struct m0_clovis_idx idx;
struct m0_clovis_realm     clovis_uber_realm; 

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

int m0_get_kvs(struct m0_clovis_idx *idx, char *k, size_t klen,
	       char *v, size_t vlen)
{
        struct m0_bufvec keys;
        struct m0_bufvec vals;
        struct m0_clovis_op *op = NULL;
        int rc;

        rc = m0_bufvec_alloc(&keys, 1, KLEN) ?:
             m0_bufvec_empty_alloc(&vals, 1);

        assert(rc == 0);
	memcpy(keys.ov_buf[0], k, klen);

        rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_GET,
                              &keys, &vals, &op);
	if (rc != 0)
		goto exit;

        m0_clovis_op_launch(&op, 1);
        rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
                               m0_time_from_now(3,0));
	printf("rc=%d\n", rc);
	if (rc != 0)
		goto exit;

        printf( "V=%s\n", (char*)vals.ov_buf[0]);
	memcpy(v, (char *)vals.ov_buf[0], vlen);

        m0_bufvec_free(&keys);
        m0_bufvec_free(&vals);
        m0_clovis_op_fini(op);
        m0_free0(&op);

exit:
        m0_bufvec_free(&keys);
        m0_bufvec_free(&vals);
        m0_clovis_op_fini(op);
        m0_free0(&op);

	return rc;
}

int m0_set_kvs(struct m0_clovis_idx *idx, char *k, size_t klen, 
	       char *v, size_t vlen)
{
        struct m0_bufvec keys;
        struct m0_bufvec vals;
        struct m0_clovis_op *op = NULL;
        int rc;

        rc = m0_bufvec_alloc(&keys, 1, KLEN) ?:
             m0_bufvec_alloc(&vals, 1, VLEN);

        assert(rc == 0);
	memcpy(keys.ov_buf[0], k, klen);
	memcpy(vals.ov_buf[0], v, vlen);

        rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_PUT,
                              &keys, &vals, &op);
	if (rc != 0)
		goto exit;

        m0_clovis_op_launch(&op, 1);
        rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
                               m0_time_from_now(3,0));
	printf("rc=%d\n", rc);
	if (rc != 0 && rc != -3)
		goto exit;

        m0_clovis_op_fini(op);
        m0_free0(&op);

	if (rc == -3) {
		/* Quick & Dirty workaround */
		printf("VIA WORKAROUND\n");

		rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_DEL, &keys, NULL, &op);
        	assert(rc == 0);
        	m0_clovis_op_launch(&op, 1);
	        rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
        	                       m0_time_from_now(3,0));
		printf("rc=%d\n", rc);
		if (rc != 0)
			goto exit;

        	m0_clovis_op_fini(op);
  	        m0_free0(&op);

        	rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_PUT,
                	              &keys, &vals, &op);
        	m0_clovis_op_launch(&op, 1);
	        rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
        	                       m0_time_from_now(3,0));
		printf("rc=%d\n", rc);
		if (rc != 0)
			goto exit;

        	m0_clovis_op_fini(op);
  	        m0_free0(&op);
		
	}
exit:
        m0_bufvec_free(&keys);
        m0_bufvec_free(&vals);

	return rc;
}

int m0_del_kvs(struct m0_clovis_idx *idx, char *k)
{
        struct m0_bufvec keys;
        struct m0_clovis_op *op = NULL;
        int rc;

        rc = m0_bufvec_alloc(&keys, 1, KLEN);

        if (rc != 0);
		return rc; 

	rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_DEL, &keys, NULL, &op);
	if (rc != 0)
		goto exit;

        m0_clovis_op_launch(&op, 1);
        rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
                               m0_time_from_now(3,0));
	printf("rc=%d\n", rc);
	if (rc != 0)
		goto exit;

exit:
        m0_bufvec_free(&keys);
        m0_clovis_op_fini(op);
        m0_free0(&op);

	return rc;
}

#define CNT 10
void m0_iter_kvs(struct m0_clovis_idx *idx, char *k)
{
	struct m0_bufvec           keys;
	struct m0_bufvec           vals;
	struct m0_clovis_op       *op = NULL;
	int i = 0;
	int rc;
	bool stop = false;
	char myk[KLEN];


	strcpy(myk, k);

	do {
	        /* Iterate over all records in the index. */
        	rc = m0_bufvec_empty_alloc(&keys, CNT) ?:
	             m0_bufvec_empty_alloc(&vals, CNT);
		assert(rc == 0);

	        keys.ov_buf[0] = m0_alloc(strlen(myk)+1);
	        keys.ov_vec.v_count[0] = strlen(myk)+1;
		strcpy(keys.ov_buf[0], myk);

	        rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_NEXT, &keys, &vals, &op);
		assert(rc == 0);

	        m0_clovis_op_launch(&op, 1);
        	rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
                	               m0_time_from_now(3,0));
		assert(rc == 0);

		printf("keys.ov_vec.v_nr =%d\n", keys.ov_vec.v_nr);
		for (i = 0; i < keys.ov_vec.v_nr ; i++) {
			if (keys.ov_buf[i] == NULL) {
				stop = true;
				break;
			}

			/* Avoid last one and use it as first of next pass */
			if ((i != keys.ov_vec.v_nr -1) && !stop)
				printf("%d, key=%s vals=%s\n", i,
			       		(char *)keys.ov_buf[i], (char *)vals.ov_buf[i]);

			strcpy(myk, (char *)keys.ov_buf[i]);
		}

		m0_bufvec_free(&keys);
		m0_bufvec_free(&vals);
		m0_clovis_op_fini(op);
		m0_free0(&op);

	} while (!stop); 
}

void m0_pattern_kvs(struct m0_clovis_idx *idx, char *k, char *pattern)
{
        struct m0_bufvec           keys;
        struct m0_bufvec           vals;
        struct m0_clovis_op       *op = NULL;
        int i = 0;
        int rc;
        bool stop = false;
        char myk[KLEN];
	bool startp = false;

        strcpy(myk, k);

        do {
                /* Iterate over all records in the index. */
                rc = m0_bufvec_empty_alloc(&keys, CNT) ?:
                     m0_bufvec_empty_alloc(&vals, CNT);
                assert(rc == 0);

                keys.ov_buf[0] = m0_alloc(strlen(myk)+1);
                keys.ov_vec.v_count[0] = strlen(myk)+1;
                strcpy(keys.ov_buf[0], myk);

                rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_NEXT, &keys, &vals, &op);
                assert(rc == 0);

                m0_clovis_op_launch(&op, 1);
                rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
                                       m0_time_from_now(3,0));
                assert(rc == 0);

                printf("keys.ov_vec.v_nr =%d\n", keys.ov_vec.v_nr);
                for (i = 0; i < keys.ov_vec.v_nr ; i++) {
                        if (keys.ov_buf[i] == NULL) {
                                stop = true;
                                break;
                        }

			/* Small state machine to display things (they are sorted) */
			if (!fnmatch(pattern, (char *)keys.ov_buf[i], 0)) {

				/* Avoid last one and use it as first of next pass */
				if ((i != keys.ov_vec.v_nr -1) && !stop)
                        		printf("%d, key=%s vals=%s\n", i,
                               			(char *)keys.ov_buf[i], (char *)vals.ov_buf[i]);

				if (startp == false)
					startp = true;
			} else {
				if (startp == true) {
					printf("Stop by pattern\n");
					stop = true;
					break;
				}
			}

                        strcpy(myk, (char *)keys.ov_buf[i]);
                }

                /* /!\ myk appears twice : last of former list and 1st of new one */

                m0_bufvec_free(&keys);
                m0_bufvec_free(&vals);
                m0_clovis_op_fini(op);
                m0_free0(&op);

        } while (!stop);
}

void m0_list_kvs(struct m0_clovis_idx *idx)
{
	int rc;
	struct m0_clovis_op       *op = NULL;
        struct m0_bufvec           keys;

	/* List all indices (only one exists). */
	rc = m0_bufvec_alloc(&keys, 2, sizeof(struct m0_fid));
	assert(rc == 0);

	rc = m0_clovis_idx_op(idx, M0_CLOVIS_IC_LIST, &keys, NULL, &op);
	assert(rc == 0);

        m0_clovis_op_launch(&op, 1);
        rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
                               m0_time_from_now(3,0));
        assert(rc == 0);
        assert(keys.ov_vec.v_nr == 2);
        assert(keys.ov_vec.v_count[0] == sizeof(struct m0_fid));
        //assert(m0_fid_eq(keys.ov_buf[0], &ifid));
        assert(keys.ov_vec.v_count[1] == sizeof(struct m0_fid));

        m0_clovis_op_fini(op);
        m0_free0(&op);
}


/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scro
 *  ll-step: 1
 *  End:
 */
