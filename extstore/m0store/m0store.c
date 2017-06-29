/* -*- C -*- */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syscall.h> /* for gettid */
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"
#include "lib/thread.h"
#include "m0store.h"

/* To be passed as argument */
extern struct m0_clovis_realm     clovis_uber_realm; 

static pthread_once_t clovis_init_once = PTHREAD_ONCE_INIT;
bool clovis_init_done = false;
__thread struct m0_thread m0thread;
__thread bool my_init_done = false;

static pthread_t m0_init_thread;

struct clovis_io_ctx {
        struct m0_indexvec ext;
        struct m0_bufvec   data;
        struct m0_bufvec   attr;
};

static char *clovis_local_addr;
static char *clovis_ha_addr;
static char *clovis_prof;
static char *clovis_proc_fid;
static char *clovis_index_dir = "/tmp/";

/* Clovis Instance */
static struct m0_clovis          *clovis_instance = NULL;

/* Clovis container */
static struct m0_clovis_container clovis_container;

/* Clovis Configuration */
static struct m0_clovis_config    clovis_conf;

struct m0_clovis_realm     clovis_uber_realm;

static void get_clovis_env(m0store_config_t *m0store_config)
{
        clovis_local_addr = getenv("CLOVIS_LOCAL_ADDR");
        assert(clovis_local_addr != NULL);

        clovis_ha_addr = getenv("CLOVIS_HA_ADDR");
        assert(clovis_ha_addr != NULL);

        clovis_prof = getenv("CLOVIS_PROFILE");
        assert(clovis_prof != NULL);

	clovis_proc_fid = getenv("CLOVIS_PROC_FID");
	assert(clovis_proc_fid != NULL);

	strcpy(m0store_config->clovis_local_addr, clovis_local_addr);
	strcpy(m0store_config->clovis_ha_addr, clovis_ha_addr);
	strcpy(m0store_config->clovis_prof, clovis_prof);
	strcpy(m0store_config->clovis_proc_fid, clovis_proc_fid);
	strcpy(m0store_config->clovis_index_dir, "/tmp");
}

static int init_ctx(struct clovis_io_ctx *ioctx, off_t off,
                    int block_count, int block_size)
{
	int                rc;
	int                i;
	uint64_t           last_index;

	/* Allocate block_count * 4K data buffer. */
	rc = m0_bufvec_alloc(&ioctx->data, block_count, block_size);
	if (rc != 0)
		return rc;

	/* Allocate bufvec and indexvec for write. */
	rc = m0_bufvec_alloc(&ioctx->attr, block_count, 1);
	if(rc != 0)
		return rc;

	rc = m0_indexvec_alloc(&ioctx->ext, block_count);
	if (rc != 0)
		return rc;

	last_index = off;
	for (i = 0; i < block_count; i++) {
		ioctx->ext.iv_index[i] = last_index ;
		ioctx->ext.iv_vec.v_count[i] = block_size;
		last_index += block_size;

		/* we don't want any attributes */
		ioctx->attr.ov_vec.v_count[i] = 0;
	}

	return 0;
}

int m0store_create_object(struct m0_uint128 id)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	if (!my_init_done)
		m0store_init();

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_default_layout_id(clovis_instance));

	m0_clovis_entity_create(&obj.ob_entity, &ops[0]);

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		M0_TIME_NEVER);

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

int m0store_delete_object(struct m0_uint128 id)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	if (!my_init_done)
		m0store_init();

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_default_layout_id(clovis_instance));

	m0_clovis_entity_delete(&obj.ob_entity, &ops[0]);

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		M0_TIME_NEVER);

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}


static int write_data_aligned(struct m0_uint128 id, char *buff, off_t off,
                                int block_count, int block_size)
{
	int                  rc;
	int                  op_rc;
	int                  i;
	int                  nr_tries = 10;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};
	struct clovis_io_ctx    ioctx;

	printf("write_data_aligned: offset=%lld, bcount=%u bs=%u\n",
		(long long)off, block_count, block_size);

	if (!my_init_done)
		m0store_init();

again:
	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	init_ctx(&ioctx, off, block_count, block_size);

	for (i = 0; i < block_count; i++)
		memcpy(ioctx.data.ov_buf[i],
		       (char *)(buff+i*block_size),
		       block_size);

	/* Set the  bject entity we want to write */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_default_layout_id(clovis_instance));

	/* Create the write request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_WRITE,
			 &ioctx.ext, &ioctx.data, &ioctx.attr, 0, &ops[0]);

	/* Launch the write request*/
	m0_clovis_op_launch(ops, 1);

	/* wait */
	rc = m0_clovis_op_wait(ops[0],
			       M0_BITS(M0_CLOVIS_OS_FAILED,
			       M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	op_rc = ops[0]->op_sm.sm_rc;

	/* fini and release */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	if (op_rc == -EINVAL && nr_tries != 0) {
		nr_tries--;
		ops[0] = NULL;
		printf("try writing data to object again ... \n");
		sleep(5);
		goto again;
	}

	/* Free bufvec's and indexvec's */
	m0_indexvec_free(&ioctx.ext);
	m0_bufvec_free(&ioctx.data);
	m0_bufvec_free(&ioctx.attr);

	/*
	 *    /!\    /!\    /!\    /!\
	 *
	 * As far as I have understood, MERO does the IO in full
	 * or does nothing at all, so returned size is aligned sized */
	return (block_count*block_size);
}

static int read_data_aligned(struct m0_uint128 id,
                             char *buff, off_t off, 
                             int block_count, int block_size)
{
	int                     i;
	int                     rc;
	struct m0_clovis_op    *ops[1] = {NULL};
	struct m0_clovis_obj    obj;
	uint64_t                last_index;
	struct clovis_io_ctx ioctx;

	printf("read_data_aligned: offset=%lld, bcount=%u bs=%u\n",
		(long long)off, block_count, block_size);

	if (!my_init_done)
		m0store_init();

	rc = m0_indexvec_alloc(&ioctx.ext, block_count);
	if (rc != 0)
		return rc;

	rc = m0_bufvec_alloc(&ioctx.data, block_count, block_size);
	if (rc != 0)
		return rc;
	rc = m0_bufvec_alloc(&ioctx.attr, block_count, 1);
	if(rc != 0)
		return rc;

	last_index = off;
	for (i = 0; i < block_count; i++) {
		ioctx.ext.iv_index[i] = last_index ;
		ioctx.ext.iv_vec.v_count[i] = block_size;
		last_index += block_size;

		ioctx.attr.ov_vec.v_count[i] = 0;
	}

	/* Read the requisite number of blocks from the entity */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_default_layout_id(clovis_instance));

	/* Create the read request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_READ,
			 &ioctx.ext, &ioctx.data, &ioctx.attr,
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
	printf("=======> op_sm.sm_state=%d M0_CLOVIS_OS_STABLE=%d\n",
		ops[0]->op_sm.sm_state, M0_CLOVIS_OS_STABLE);
	assert(ops[0]->op_sm.sm_state == M0_CLOVIS_OS_STABLE);
	assert(ops[0]->op_sm.sm_rc == 0);

	for (i = 0; i < block_count; i++)
		memcpy((char *)(buff + block_size*i),
		       (char *)ioctx.data.ov_buf[i],
		       ioctx.data.ov_vec.v_count[i]);


	/* fini and release */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	m0_indexvec_free(&ioctx.ext);
	m0_bufvec_free(&ioctx.data);
	m0_bufvec_free(&ioctx.attr);

	/*
	 *    /!\    /!\    /!\    /!\
	 *
	 * As far as I have understood, MERO does the IO in full
	 * or does nothing at all, so returned size is aligned sized */
	return (block_count*block_size);
}

static int init_clovis(m0store_config_t *m0store_config)
{
        int rc;

	get_clovis_env(m0store_config);

        /* Initialize Clovis configuration */
        clovis_conf.cc_is_oostore               = false;
        clovis_conf.cc_is_read_verify           = false;
        clovis_conf.cc_local_addr               = clovis_local_addr;
        clovis_conf.cc_ha_addr                  = clovis_ha_addr;
        clovis_conf.cc_profile                  = clovis_prof;
        clovis_conf.cc_tm_recv_queue_min_len    = M0_NET_TM_RECV_QUEUE_DEF_LEN;
        clovis_conf.cc_max_rpc_msg_size         = M0_RPC_DEF_MAX_RPC_MSG_SIZE;

        /* Index service parameters */
	clovis_conf.cc_idx_service_id   = M0_CLOVIS_IDX_MOCK;
	clovis_conf.cc_idx_service_conf  = clovis_index_dir;
	clovis_conf.cc_process_fid       = clovis_proc_fid;

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

static void m0store_do_init(void)
{
	int rc;
	m0store_config_t m0store_config;

	get_clovis_env(&m0store_config);

	rc = init_clovis(&m0store_config);
	assert(rc == 0);

	clovis_init_done = true;
	m0_init_thread = pthread_self();
}

int m0store_init(void)
{
	(void) pthread_once(&clovis_init_once, m0store_do_init);

	if (clovis_init_done && (pthread_self() != m0_init_thread)) {
		printf("==========> tid=%d I am not the init thread\n",
		       (int)syscall(SYS_gettid));

		memset(&m0thread, 0, sizeof(struct m0_thread));

		m0_thread_adopt(&m0thread, clovis_instance->m0c_mero);
	} else
		printf("----------> tid=%d I am the init thread\n",
		       (int)syscall(SYS_gettid));

	my_init_done = true;

	return 0;
}

void m0store_fini(void)
{
	if (pthread_self() == m0_init_thread) {
		/* Finalize Clovis instance */
		m0_clovis_fini(clovis_instance, true);
	} else
		m0_thread_shun();
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


/*
 * The following functions makes random IO by blocks
 *
 */

/*
 * Those two functions compute the Upper and Lower limits
 * for the block that contains the absolution offset <x>
 * For related variables will be named Lx and Ux in the code
 *
 * ----|-----------x-------|-----
 *     Lx                  Ux
 *
 * Note: Lx and Ux are multiples of the block size 
 */
static off_t lower(off_t x, size_t bs)
{
	return (x/bs)*bs;
}

static off_t upper(off_t x, size_t bs)
{
	return ((x/bs)+1)*bs;
}

/* equivalent of pwrite, but does only IO on full blocks */
ssize_t m0store_do_io(struct m0_uint128 id, enum io_type iotype,
		      off_t x, size_t len, size_t bs, char *buff)
{
	off_t Lx1, Lx2, Ux1, Ux2;
	off_t Lio, Uio, Ubond, Lbond;
	bool bprev, bnext, insider;
	off_t x1, x2;
	int bcount = 0;
	int rc;
	int delta_pos = 0;
	int delta_tmp = 0;
	ssize_t done = 0;
	char tmpbuff[M0STORE_BLK_SIZE];

	/*
	 * IO will not be considered the usual offset+len way
	 * but as segment starting from x1 to x2
	 */
	x1 = x;
	x2 = x+len;

	/* Compute Lower and Upper Limits for IO */
	Lx1 = lower(x1, bs);
	Lx2 = lower(x2, bs);
	Ux1 = upper(x1, bs);
	Ux2 = upper(x2, bs);

	/* Those flags preserve state related to the way
	 * the IO should be done.
	 * - insider is true : x1 and x2 belong to the 
	 *   same block (the IO is fully inside a single block)
	 * - bprev is true : x1 is not a block limit
	 * - bnext is true : x2 is not a block limit
	 */
	bprev = false;
	bnext = false;
	insider = false;

	/* If not an "insider case", the IO can be made in 3 steps
	 * a) inside [x1,x2], find a set of contiguous aligned blocks
	 *    and do the IO on them
	 * b) if x1 is not aligned on block size, do a small IO on the
	 *    block just before the "aligned blocks"
	 * c) if x2 is not aligned in block size, do a small IO on the
	 *    block just after the "aligned blocks"
	 *
	 * Example: x1 and x2 are located so
	 *           x <--------------- len ------------------>
	 *  ---|-----x1-----|------------|------------|-------x2--|----
	 *     Lx1          Ux1                       Lx2         Ux2
	 *
	 * We should (write case)
	 *   1) read block [Lx1, Ux1] and update range [x1, Ux1]
	 *     then write updated [Lx1, Ux1]
	 *   3) read block [Lx2, Ux2], update [Lx2, x2] and
	 *       then writes back updated [Lx2, Ux2]
	 */
	printf("IO: (%lld, %llu) = [%lld, %lld]\n",
		(long long)x, (unsigned long long)len,
		(long long)x1, (long long)x2);

	printf("  Bornes: %lld < %lld < %lld ||| %lld < %lld < %lld\n",
		(long long)Lx1, (long long)x1, (long long)Ux1,
		(long long)Lx2, (long long)x2, (long long)Ux2);

	/* In the following code, the variables of interest are:
	 *  - Lio and Uio are block aligned offset that limit
	 *    the "aligned blocks IO"
	 *  - Ubond and Lbound are the Up and Low limit for the
	 *    full IO, showing every block that was touched. It is
	 *    used for debug purpose */
	if ((Lx1 == Lx2) && (Ux1 ==  Ux2)) {
		/* Insider case, x1 and x2 are so :
		 *  ---|-x1---x2----|---
		 */
		bprev = bnext = false;

		insider = true;
		Lio = Uio = 0LL;
		Ubond = Ux1;
		Lbond = Lx1;
	} else {
		/* Left side */
		if (x1 == Lx1) {
			/* Aligned on the left
			* --|------------|----
			*   x1
			*   Lio
			*   Lbond
			*/
			Lio = x1;
			bprev = false;
			Lbond = x1;
		} else {
			/* Not aligned on the left
			* --|-----x1------|----
			*                 Lio
			*   Lbond
			*/
			Lio = Ux1;
			bprev = true;
			Lbond = Lx1;
		}

		/* Right side */
		if (x2 == Lx2) {
			/* Aligned on the right
			* --|------------|----
			*                x2
			*                Uio
			*                Ubond
			*/
			Uio = x2;
			bnext = false;
			Ubond = x2;
		} else {
			/* Not aligned on the left
			* --|---------x2--|----
			*   Uio
			*                 Ubond
			*/
			Uio = Lx2;
			bnext = true;
			Ubond = Ux2;
		}
	}

	/* delta_pos is the offset position in input buffer "buff"
	 * What is before buff+delta_pos has already been done */
	delta_pos = 0;

	printf("IO on [%lld, %lld]:\n", (long long)x1, (long long)x2);
	if (bprev) {
		printf("PREVIOUS: Get block [%lld, %lld] and work on [%lld, %lld]\n",
			(long long)Lx1, (long long)Ux1,
			(long long)x1, (long long)Ux1);

		/* Reads block [Lx1, Ux1] before aligned [Lio, Uio] */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx1, 1, bs);
		if (rc < 0 || rc != bs)
			return -1;

		/* Update content of read block
		 * --|-----------------------x1-----------|---
		 *   Lx1                                  Ux1
		 *                              WORK HERE
		 *    <----------------------><---------->
		 *          x1-Lx1                Ux1-x1
		 */
		delta_tmp = x1 - Lx1;
		switch(iotype) {
		case IO_WRITE:
			memcpy((char *)(tmpbuff+delta_tmp),
			       buff, (Ux1 - x1));

			/* Writes block [Lx1, Ux1] once updated */
			rc = write_data_aligned(id, tmpbuff, Lx1, 1, bs);
			if (rc < 0 || rc != bs)
				return -1;

			break;

		case IO_READ:
			 memcpy(buff, (char *)(tmpbuff+delta_tmp),
			       (Ux1 - x1));
			break;

		default:
			return -EINVAL;
		}

		delta_pos += Ux1 - x1;
		done += Ux1 - x1;
	}

	if (Lio != Uio) {
		/* Easy case: aligned IO on aligned limit [Lio, Uio] */
		/* If no aligned block were found, then Uio == Lio */
		printf("ALIGNED: do IO on [%lld, %lld]\n",
			(long long)Lio, (long long)Uio);

		bcount = (Uio - Lio)/bs;
		switch(iotype) {
		case IO_WRITE:
			rc = write_data_aligned(id, (char *)(buff + delta_pos),
						Lio, bcount, bs);

			if (rc < 0)
				return -1;
			break;

		case IO_READ:
			rc = read_data_aligned(id, (char *)(buff + delta_pos),
					       Lio, bcount, bs);

			if (rc < 0)
				return -1;
			break;

		default:
			return -EINVAL;
		}

		if (rc != (bcount*bs))
			return -1;

		done += rc;
		delta_pos += done;
	}

	if (bnext) {
		printf("NEXT: Get block [%lld, %lld] and work on [%lld, %lld]\n",
		       (long long)Lx2, (long long)Ux2,
		       (long long)Lx2, (long long)x2);

		/* Reads block [Lx2, Ux2] after aligned [Lio, Uio] */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx2, 1, bs);
		if (rc < 0)
			return -1;

		/* Update content of read block
		 * --|---------------x2------------------|---
		 *   Lx2                                 Ux2
		 *       WORK HERE
		 *    <--------------><------------------>
		 *          x2-Lx2           Ux2-x2
		 */
		switch(iotype) {
		case IO_WRITE:
			memcpy(tmpbuff, (char *)(buff + delta_pos),
			      (x2 - Lx2));

			/* Writes block [Lx2, Ux2] once updated */
			/* /!\ This writes extraenous ending zeros */
			rc = write_data_aligned(id, tmpbuff, Lx2, 1, bs);
			if (rc < 0)
				return -1;
			break;

		case IO_READ:
			memcpy((char *)(buff + delta_pos), tmpbuff,
			       (x2 - Lx2));
			break;

		default:
			return -EINVAL;
		}

		done += x2 - Lx2;
	}

	if (insider) {
		printf("INSIDER: Get block [%lld, %lld] and work on [%lld, %lld]\n",
			(long long)Lx1, (long long)Ux1,
			(long long)x1, (long long)x2);

		/* Insider case read/update/write */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx1, 1, bs);
		if (rc < 0)
			return -1;

		/* --|----------x1---------x2------------|---
		 *   Lx1=Lx2                             Ux1=Ux2
		 *                  UPDATE
		 *    <---------><---------->
		 *       x1-Lx1      x2-x1
		 */
		delta_tmp = x1 - Lx1;
		switch(iotype) {
		case IO_WRITE:
			memcpy((char *)(tmpbuff+delta_tmp), buff,
			       (x2 - x1));

			/* /!\ This writes extraenous ending zeros */
			rc = write_data_aligned(id, tmpbuff, Lx1, 1, bs);
			if (rc < 0)
				return -1;
			break;

		case IO_READ:
			memcpy(buff, (char *)(tmpbuff+delta_tmp),
			       (x2 - x1));
			break;

		default:
			return -EINVAL;
		}

		done += x2 - x1;
	}

	printf("Complete IO : [%lld, %lld] => [%lld, %lld]\n",
		(long long)x1, (long long)x2,
		(long long)Lbond, (long long)Ubond);

	printf("End of IO : len=%llu  done=%lld\n\n",
	       (long long)len, (long long)done);

	return done;
}

