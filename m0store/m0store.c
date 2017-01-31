/* -*- C -*- */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

/* To be passed as argument */
extern struct m0_clovis_realm     clovis_uber_realm; 

struct clovis_io_ctx {
        struct m0_indexvec ext;
        struct m0_bufvec   data;
        struct m0_bufvec   attr;
};

static int init_ctx(struct clovis_io_ctx *ioctx,
                    int block_size,
                    int block_count)
{
        int rc;
        int i;
	uint64_t                last_index = 0LL;

        /* Allocate block_count * 4K data buffer. */
        rc = m0_bufvec_empty_alloc(&ioctx->data, block_count);
        if (rc != 0)
                return rc;

        /* Allocate bufvec and indexvec for write. */
        rc = m0_bufvec_alloc(&ioctx->attr, block_count, 1);
        if(rc != 0)
                return rc;

        rc = m0_indexvec_alloc(&ioctx->ext, block_count);
        if (rc != 0)
                return rc;

        for (i = 0; i < block_count; i++) {
                ioctx->ext.iv_index[i] = last_index;
                ioctx->ext.iv_vec.v_count[i] = block_size;
                last_index += block_size;

                /* we don't want any attributes */
                ioctx->attr.ov_vec.v_count[i] = 0;
        }

        return 0;
};

static void free_ctx(struct clovis_io_ctx *ioctx)
{
        m0_indexvec_free(&ioctx->ext);
        m0_bufvec_free(&ioctx->data);
        m0_bufvec_free(&ioctx->attr);
}

static int read_data_from_object(struct m0_uint128 id,
				 struct clovis_io_ctx *ioctx)
{
	int rc = 0;
	struct m0_clovis_op    *ops[1] = {NULL};
	struct m0_clovis_obj    obj;

	/* Read the requisite number of blocks from the entity */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);
	/* Create the read request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_READ,
			 &ioctx->ext, &ioctx->data, &ioctx->attr,
			 0, &ops[0]);
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

	/* fini and release */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

int m0_pread(struct m0_uint128 id, char *buff, int block_size, int block_count)
{
	int                     i;
	int                     rc;
	struct clovis_io_ctx    ioctx;


        /* Read data from source file. */
        rc = init_ctx(&ioctx, block_size, block_count);
        if (rc != 0)
                return rc;

        for (i = 0; i < block_count; i++) {
                ioctx.data.ov_buf[i] = (char *)(buff+i*block_size);
                ioctx.data.ov_vec.v_count[i] = block_size;
        }

	rc = read_data_from_object(id, &ioctx);
	if (rc != 0) {
		fprintf(stderr, "Reading from object failed rc=%d\n", rc);
		return rc;
	}

        /* Free bufvec's and indexvec's */
        for (i = 0; i < block_count; i++) {
                ioctx.data.ov_buf[i] = NULL;
                ioctx.data.ov_vec.v_count[i] = 0;
        }
	free_ctx(&ioctx);
	
	return 0;
}

int m0store_create_object(struct m0_uint128 id)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);

	m0_clovis_entity_create(&obj.ob_entity, &ops[0]);

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		m0_time_from_now(3,0));
	
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

static int write_data_to_object(struct m0_uint128 id,
				struct clovis_io_ctx *ioctx)
{
	int                  rc;  
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	/* Set the object entity we want to write */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id);

	/* Create the write request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_WRITE, 
			 &ioctx->ext, &ioctx->data, &ioctx->attr,
			 0, &ops[0]);

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

int m0_pwrite(struct m0_uint128 id, char *buff, int block_size, int block_count)
{ 
	struct clovis_io_ctx ioctx;
	int i = 0;
	int rc = 0;

	/* Read data from source file. */
	rc = init_ctx(&ioctx, block_size, block_count);
	if (rc != 0)
		return rc;

	for (i = 0; i < block_count; i++) {
		ioctx.data.ov_buf[i] = (char *)(buff+i*block_size);
		ioctx.data.ov_vec.v_count[i] = block_size;
	}

	/* Copy data to the object*/
	rc = write_data_to_object(id, &ioctx);
	if (rc != 0) {
		fprintf(stderr, "Writing to object failed!\n");
		return rc;
	}
	
	
	/* Free bufvec's and indexvec's */
	for (i = 0; i < block_count; i++) {
		ioctx.data.ov_buf[i] = NULL;
		ioctx.data.ov_vec.v_count[i] = 0;
	}
	free_ctx(&ioctx);
	
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
