/* -*- C -*- */
/*
 * COPYRIGHT 2014 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE LLC,
 * ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.xyratex.com/contact
 *
 * Original author:  Juan   Gonzalez <juan.gonzalez@seagate.com>
 *                   James  Morse    <james.s.morse@seagate.com>
 *                   Sining Wu       <sining.wu@seagate.com>
 * Original creation date: 30-Oct-2014
 */
/* This is a sample Clovis application. It will read data from
 * a file and write it to an object.
 * In General, steps are:
 * 1. Create a Clovis instance from the configuration data provided.
 * 2. Create an object.
 * 3. Read data from a file and fill it into Clovis buffers.
 * 4. Submit the write request.
 * 5. Wait for the write request to finish.
 * 6. Finalise the Clovis instance.
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>

#include <clovis/clovis.h>
#include <clovis/clovis_internal.h> /* required for m0c_mero */
#include <clovis/clovis_idx.h>
#include <libzfswrap.h>
#include "extstore.h"


/* Currently Clovis can write at max 200 blocks in
 * a single request. This will change in future. */
#define CLOVIS_MAX_BLOCK_COUNT (200)
#define BLK_SIZE 4096

struct io_desc {
	unsigned int fbp; /** 1st block position */
	unsigned int ofb; /** Offset in 1st block */
	unsigned int ncb; /** nb of contiguous block */
	unsigned int olb; /** offset in last block */

	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
};

/* Clovis parameters */
static int   clovis_block_size = 4096;

/* Clovis Instance */
static struct m0_clovis          *clovis_instance = NULL;

/* Clovis container */
static struct m0_clovis_container clovis_container;
static struct m0_clovis_realm     clovis_uber_realm;

/* Clovis Configuration */
static struct m0_clovis_config    clovis_conf;

static pthread_t init_threadid;
__thread bool clovis_init_done = false; /* To be kept in TLS */

/* 
 * This function initialises Clovis and Mero.
 * Creates a Clovis instance and initializes the 
 * realm to uber realm.
 */
int init_clovis(char *clovis_local_addr,
	        char *clovis_ha_addr,
		char *clovis_confd_addr,
		char *clovis_prof,
		char *clovis_index_dir)
{
	int rc;

	/* Remeber who did the job */
	init_threadid = pthread_self();

	/* Initialize Clovis configuration */
	clovis_conf.cc_is_oostore            = false;
	clovis_conf.cc_is_read_verify        = false;
	clovis_conf.cc_local_addr            = clovis_local_addr;
	clovis_conf.cc_ha_addr               = clovis_ha_addr;
	clovis_conf.cc_confd                 = clovis_confd_addr;
	clovis_conf.cc_profile               = clovis_prof;
	clovis_conf.cc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
	clovis_conf.cc_max_rpc_msg_size      = M0_RPC_DEF_MAX_RPC_MSG_SIZE;

	/* Index service parameters */
	clovis_conf.cc_idx_service_id        = M0_CLOVIS_IDX_MOCK;
        clovis_conf.cc_idx_service_conf      = clovis_index_dir;

	/* Create Clovis instance */
	rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);
	if (rc != 0) {
		printf("Failed to initilise Clovis\n");
		goto err_exit;
	}

	/* Container is where Entities (object) resides.
 	 * Currently, this feature is not implemented in Clovis.
 	 * We have only single realm: UBER REALM. In future with multiple realms
 	 * multiple applications can run in different containers. */
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
	/* Finalize Clovis instance */
	m0_clovis_fini(&clovis_instance, true);
}

#include "lib/thread.h"
void thread_enroll()
{
	struct m0 *m0;
	struct m0_thread *mthread;

	if (clovis_init_done)
		return;

	/* Do not enroll main thread */
	printf( "ME=%llu INIT=%llu\n", (unsigned long long)pthread_self(),
		(unsigned long long)init_threadid);
	if (pthread_self() == init_threadid) {
		printf( "Main thread, do not enroll\n");
		return;
	}

	m0 = clovis_instance->m0c_mero;

	mthread = malloc(sizeof(struct m0_thread));
	if (mthread == NULL) {
		fprintf(stderr, "No more merory\n");
		abort();
	}
	memset(mthread, 0, sizeof(struct m0_thread));

	printf("Enrolling thread in MERO\n");

	clovis_init_done = true;

	m0_thread_adopt(mthread, m0);

}

void thread_quit()
{
	/* We should think about releasing m0_thread structures too */
	m0_thread_shun();
}

int handle2m0_id(inogen_t *ino , struct m0_uint128 *object_id)
{
	if (!ino || !object_id)
		return -1; 

	*object_id = M0_CLOVIS_ID_APP; /* Init */
	object_id->u_lo = ino->inode; 
	object_id->u_hi = ino->generation;

	return 0;
}

int create_object(inogen_t *ino)
{
	int                  rc;

	/* Clovis object */
	struct m0_clovis_obj obj;
	struct m0_uint128 id;

	/* Clovis operation */
	struct m0_clovis_op *ops[1] = {NULL};

	if (handle2m0_id(ino, &id) < 0)
		return -1;

	thread_enroll();

	memset(&obj, 0, sizeof(struct m0_clovis_obj));
 
 	/* Initialize obj structures 
 	 * Note: This api doesnot create an object. It simply fills
 	 * obj structure with require data. */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);

	/* Create object-create request */
	m0_clovis_entity_create(&obj.ob_entity, &ops[0]);

       /* Launch the request. This is a asynchronous call.
 	* This will actually create an object */
	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	/* Wait for the object creation to finish */
	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		m0_time_from_now(3,0));
	
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	printf( "OBJECT CREATED rc=%d !!!\n", rc);

	return rc;
}

int delete_object(inogen_t *ino)
{
	int                  rc;

	/* Clovis object */
	struct m0_clovis_obj obj;
	struct m0_uint128 id;

	/* Clovis operation */
	struct m0_clovis_op *ops[1] = {NULL};

	thread_enroll();

	if (handle2m0_id(ino, &id) < 0)
		return -1;

	memset(&obj, 0, sizeof(struct m0_clovis_obj));
 
 	/* Initialize obj structures 
 	 * Note: This api doesnot create an object. It simply fills
 	 * obj structure with require data. */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);

	/* Create object-create request */
	m0_clovis_entity_delete(&obj.ob_entity, &ops[0]);

       /* Launch the request. This is a asynchronous call.
 	* This will actually create an object */
	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	/* Wait for the object creation to finish */
	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		m0_time_from_now(3,0));
	
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	printf( "OBJECT_DELETED rc=%d !!!\n", rc);

	return rc;
}

static int build_io_desc(off_t offset, size_t length,
		         struct  io_desc *iod)
{
	if (!iod)
		return EINVAL;

	iod->fbp = offset/BLK_SIZE;
	iod->ofb = offset - (iod->fbp*BLK_SIZE);

	iod->ncb = (offset + length - (iod->fbp*BLK_SIZE))/BLK_SIZE + 1;
	iod->olb = (offset + length) - BLK_SIZE*(iod->fbp + iod->ncb -1);

	printf( "off=%llu|len=%llu => fdb=%u ofb=%u ncb=%u olb=%u\n",
	       (long long unsigned)offset, (long long unsigned)length, iod->fbp, iod->ofb, iod->ncb, iod->olb);

	return 0;
}

static int read_data_from_memory(char *b, struct io_desc *iod, char **nb)
{
        int i;
        int nr_blocks;
        char *buff = NULL;

        buff = b;

        nr_blocks = iod->data.ov_vec.v_nr;
        for (i = 0; i < nr_blocks; i++) {
                memcpy(iod->data.ov_buf[i], buff, clovis_block_size);
                buff += clovis_block_size;
        }

        *nb = buff;

        return i;
}

static int write_data_to_object(struct m0_uint128  id, struct io_desc *iod)
{
        int                  rc;
        struct m0_clovis_obj obj;
        struct m0_clovis_op *ops[1] = {NULL};

        memset(&obj, 0, sizeof(struct m0_clovis_obj));

        /* Set the object entity we want to write */
        m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);

        /* Create the write request */
        m0_clovis_obj_op(&obj, M0_CLOVIS_OC_WRITE,
                         &iod->ext, &iod->data, &iod->attr, 0, &ops[0]);

        /* Launch the write request*/
        m0_clovis_op_launch(ops, 1);

        /* wait */
        rc = m0_clovis_op_wait(ops[0],
                        M0_BITS(M0_CLOVIS_OS_FAILED,
                                M0_CLOVIS_OS_STABLE),
                        M0_TIME_NEVER);

        /* fini and release */
        m0_clovis_op_fini(ops[0]);
        m0_clovis_op_free(ops[0]);
        m0_clovis_entity_fini(&obj.ob_entity);

        return rc;
}

static void free_io_desc(struct io_desc *iod)
{
	m0_indexvec_free(&iod->ext);
	m0_bufvec_free(&iod->data);
	m0_bufvec_free(&iod->attr);
}

static int alloc_io_desc(struct io_desc *iod)
{
	int rc ;

	if (iod == NULL)
		return -1;

	rc = m0_indexvec_alloc(&iod->ext, iod->ncb);
	if (rc != 0)
		return rc;

	rc = m0_bufvec_alloc(&iod->data, iod->ncb,
			     clovis_block_size);
	if (rc != 0)
		return rc;

	rc = m0_bufvec_alloc(&iod->attr, iod->ncb, 1);
	if(rc != 0)
		return rc;

	return 0;
}

int pread_clovis(inogen_t *ino,
		 off_t 	offset,
		 size_t length,
		 char *mydata)
{
        int                     i;
        int                     rc;

        /* Object id */
        struct m0_uint128       id;
        struct m0_clovis_op    *ops[1] = {NULL};
        struct m0_clovis_obj    obj;
        uint64_t                last_index;
	struct io_desc          iod;
	char *buff = NULL;

	if (build_io_desc(offset, length, &iod))
		return -1;

        if (handle2m0_id(ino, &id) < 0)
                return -1;

	thread_enroll();

	rc = alloc_io_desc(&iod);
        if(rc != 0)
                return -1;

        last_index = iod.fbp;
        for (i = 0; i < iod.ncb; i++) {
                iod.ext.iv_index[i] = last_index ;
                iod.ext.iv_vec.v_count[i] = clovis_block_size;
                last_index += clovis_block_size;

                iod.attr.ov_vec.v_count[i] = 0;

        }

        m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);

        m0_clovis_obj_op(&obj, M0_CLOVIS_OC_READ, &iod.ext, &iod.data, &iod.attr, 0, &ops[0]);
        assert(rc == 0);
        assert(ops[0] != NULL);
        assert(ops[0]->op_sm.sm_rc == 0);

        m0_clovis_op_launch(ops, 1);

        /* wait */
        rc = m0_clovis_op_wait(ops[0],
                            M0_BITS(M0_CLOVIS_OS_FAILED,
                                    M0_CLOVIS_OS_STABLE),
                     M0_TIME_NEVER);
        assert(rc == 0);
        assert(ops[0]->op_sm.sm_state == M0_CLOVIS_OS_STABLE);
        assert(ops[0]->op_sm.sm_rc == 0);

        /* putchar the output */
	buff = mydata;
        for (i = 0; i < iod.ncb; i++) {
		memcpy(buff, (char *)iod.data.ov_buf[i], iod.data.ov_vec.v_count[i]);
		buff += iod.data.ov_vec.v_count[i];
        }

        /* fini and release */
        m0_clovis_op_fini(ops[0]);
        m0_clovis_op_free(ops[0]);
        m0_clovis_entity_fini(&obj.ob_entity);

	free_io_desc(&iod);

        return 0;
}

int pwrite_clovis(inogen_t *ino,
		  off_t 	offset,
		  size_t length,
		  char *mydata)
{
        int                i;
        int                rc;
        int                block_count;
	struct io_desc     iod;
        uint64_t           last_index;

        /* Object id */
        struct m0_uint128  id;

        char * buffer;
        char * new_buffer;

	int clovis_block_count;

        if (handle2m0_id(ino, &id) < 0)
                return -1;

	if (build_io_desc(offset, length, &iod))
		return -1;

	thread_enroll();

	clovis_block_count = iod.ncb;
	printf( "Block_count=%d\n", clovis_block_count);

        last_index = iod.fbp; // offset ??? */
        while (clovis_block_count > 0) {
                block_count = (clovis_block_count > CLOVIS_MAX_BLOCK_COUNT)?
                              CLOVIS_MAX_BLOCK_COUNT:clovis_block_count;
		iod.ncb = block_count;

                /* Allocate block_count * 4K data buffer. */
                rc = alloc_io_desc(&iod);
                if (rc != 0)
                        return rc;

                buffer = mydata;

                for (i = 0; i < block_count; i++) {
                        iod.ext.iv_index[i] = last_index ;
                        iod.ext.iv_vec.v_count[i] = clovis_block_size;
                        last_index += clovis_block_size;

                        /* we don't want any attributes */
                        iod.attr.ov_vec.v_count[i] = 0;
                }

                /* Read data from source file. */
                rc = read_data_from_memory(buffer, &iod, &new_buffer);
                buffer = new_buffer;
                assert(rc == block_count);

                /* Copy data to the object*/
                rc = write_data_to_object(id, &iod);
                if (rc != 0) {
                        fprintf(stderr, "Writing to object failed!\n");
                        return rc;
                }

                /* Free bufvec's and indexvec's */
		free_io_desc(&iod);

                clovis_block_count -= block_count;
        }

        return 0;
}


/* 
 *  Local variables: 
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
