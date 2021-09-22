/* -*- C -*- */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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

#include <kvsns/kvsal.h>
#include "m0common.h"

struct clovis_io_ctx {
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
};

/* To be passed as argument */
struct m0_realm     clovis_uber_realm;

pthread_mutex_t big_motr_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_once_t clovis_init_once = PTHREAD_ONCE_INIT;
bool clovis_init_done = false;
__thread struct m0_thread *m0thread; /* stored in the TLS */
__thread bool my_init_done = false;  /* stored in the TLS */

static pthread_t m0init_thread;
static struct collection_item *conf = NULL;

static struct m0_fid ifid;
static struct m0_idx idx;

static char *clovis_local_addr;
static char *clovis_ha_addr;
static char *clovis_prof;
static char *clovis_proc_fid;
static char *ifid_str;
static uint32_t layoutid = 0;
static bool use_m0trace = false;
static bool use_addb = false;

/* Clovis Instance */
static struct m0_client	  *clovis_instance = NULL;

/* Clovis container */
static struct m0_container clovis_container;

/* Clovis Configuration */
static struct m0_config	clovis_conf;
static struct m0_idx_dix_config	dix_conf;

struct m0_realm     clovis_uber_realm;

#define WRAP_CONFIG(__name, __cfg, __item) ({\
	int __rc = get_config_item("motr", __name, __cfg, &__item);\
	if (__rc != 0)\
		return -__rc;\
	if (__item == NULL)\
		return -EINVAL; })

/* Static functions */

static int get_clovis_conf(struct collection_item *cfg)
{
	struct collection_item *item;

	if (cfg == NULL)
		return -EINVAL;

	item = NULL;
	WRAP_CONFIG("local_addr", cfg, item);
	clovis_local_addr = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("ha_addr", cfg, item);
	clovis_ha_addr = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("profile", cfg, item);
	clovis_prof = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("proc_fid", cfg, item);
	clovis_proc_fid = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("kvs_fid", cfg, item);
	ifid_str = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("layoutid", cfg, item);
	layoutid = get_uint32_config_value(item, 0, 0, NULL);	

	item = NULL;
	WRAP_CONFIG("use_m0trace", cfg, item);
	use_m0trace = get_bool_config_value(item, 0, NULL);

	item = NULL;
	WRAP_CONFIG("use_addb", cfg, item);
	use_addb = get_bool_config_value(item, 0, NULL);

	return 0;
}

static void print_config(void)
{
	printf("local_addr  = %s\n", clovis_local_addr);
	printf("ha_addr     = %s\n", clovis_ha_addr);
	printf("profile     = %s\n", clovis_prof);
	printf("proc_fid    = %s\n", clovis_proc_fid);
	printf("kvs_fid     = %s\n", ifid_str);
	printf("layoutid    = %u\n", layoutid);
	printf("use_m0trace = %d\n", use_m0trace);
	printf("use_addb    = %d\n", use_addb);
	printf("---------------------------\n");
}

int m0kvs_reinit(void)
{
	return m0init(conf);
}

static int init_clovis(void)
{
	int rc;
	char  tmpfid[MAXNAMLEN];

	rc = get_clovis_conf(conf);
	if (rc != 0)
		return rc;

	/* Index service parameters */
	dix_conf.kc_create_meta		= false;

	/* Initialize Clovis configuration */
	clovis_conf.mc_is_oostore	= true;
	clovis_conf.mc_is_read_verify	= false;
	clovis_conf.mc_is_addb_init     = use_addb;
	clovis_conf.mc_local_addr	= clovis_local_addr;
	clovis_conf.mc_ha_addr		= clovis_ha_addr;
	clovis_conf.mc_profile		= clovis_prof;
	clovis_conf.mc_process_fid      = clovis_proc_fid;
	clovis_conf.mc_layout_id	= layoutid;
	clovis_conf.mc_tm_recv_queue_min_len    = M0_NET_TM_RECV_QUEUE_DEF_LEN;
	clovis_conf.mc_max_rpc_msg_size	 = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
	clovis_conf.mc_idx_service_id	= M0_IDX_DIX;
	clovis_conf.mc_idx_service_conf	= &dix_conf;

	/* Tell MOTR not to produce m0trace files */
	m0_trace_set_mmapped_buffer(use_m0trace);

	/* Create Clovis instance */
	rc = m0_client_init(&clovis_instance, &clovis_conf, true);
	if (rc != 0) {
		fprintf(stderr, "Failed to initilise Clovis\n");
		goto err_exit;
	}

	/* Container is where Entities (object) resides.
	 * Currently, this feature is not implemented in Clovis.
	 * We have only single realm: UBER REALM. In future with multiple realms
	 * multiple applications can run in different containers. */
	m0_container_init(&clovis_container,
				 NULL, &M0_UBER_REALM,
				 clovis_instance);

	rc = clovis_container.co_realm.re_entity.en_sm.sm_rc;
	if (rc != 0) {
		fprintf(stderr, "Failed to open uber realm\n");
		goto err_exit;
	}

	clovis_uber_realm = clovis_container.co_realm;

	/* Get fid from config parameter */
	memset(&ifid, 0, sizeof(struct m0_fid));
	rc = m0_fid_sscanf(ifid_str, &ifid);
	if (rc != 0) {
		fprintf(stderr, "Failed to read ifid value from conf\n");
		goto err_exit;
	}

	rc = m0_fid_print(tmpfid, MAXNAMLEN, &ifid);
	if (rc < 0) {
		fprintf(stderr, "Failed to read ifid value from conf\n");
		goto err_exit;
	}

	m0_idx_init(&idx, &clovis_container.co_realm,
			   (struct m0_uint128 *)&ifid);

	return 0;

err_exit:
	return rc;
}

static void m0kvs_do_init(void)
{
	int rc;

	rc = get_clovis_conf(conf);

	if (rc != 0) {
		fprintf(stderr, "Invalid config file\n");
		exit(1);
	}

	print_config();

	rc = init_clovis();
	assert(rc == 0);

	clovis_init_done = true;
	m0init_thread = pthread_self();
}


static int m0_op_kvs(enum m0_idx_opcode opcode,
		     struct m0_bufvec *key,
		     struct m0_bufvec *val)
{
	struct m0_op	 *op = NULL;
	int rcs[1];
	int rc;

	if (!my_init_done)
		m0kvs_reinit();

	rc = m0_idx_op(&idx,
		       opcode,
		       key,
		       val,
		       rcs,
		       (opcode == M0_IC_PUT) ? M0_OIF_OVERWRITE : 0,
		       &op);
	if (rc)
		return rc;

	pthread_mutex_lock(&big_motr_lock);
	m0_op_launch(&op, 1);
	pthread_mutex_unlock(&big_motr_lock);
	rc = m0_op_wait(op, M0_BITS(M0_OS_STABLE),
			       M0_TIME_NEVER);
	if (rc)
		goto out;

	/* Check rcs array even if op is succesful */
	rc = rcs[0];

out:
	m0_op_fini(op);
	/* it seems like 0_free(&op) is not needed */
	return rc;
}

static int buf2vec(char *b, size_t len, struct m0_bufvec *vec)
{
	if (!b || !vec)
		return -EINVAL;

	vec->ov_vec.v_count = malloc(sizeof(m0_bcount_t));
	vec->ov_buf = malloc(sizeof(void *));
	if (!vec->ov_vec.v_count || !vec->ov_buf) {
		/* free() accept NULL as an argument */
		free(vec->ov_vec.v_count);
		free(vec->ov_buf);
		return -ENOMEM;
	}

	vec->ov_vec.v_nr = 1;
	vec->ov_vec.v_count[0] = len;
	vec->ov_buf[0] = b;

	return 0;
}

static void free_buf2vec(struct m0_bufvec *vec)
{
	free(vec->ov_vec.v_count);
	free(vec->ov_buf);
}


/* Non-static function starts here */
static pthread_mutex_t m0init_lock = PTHREAD_MUTEX_INITIALIZER;

int m0init(struct collection_item *cfg_items)
{
	if (cfg_items == NULL)
		return -EINVAL;

	if (conf == NULL)
		conf = cfg_items;

	(void) pthread_once(&clovis_init_once, m0kvs_do_init);

	pthread_mutex_lock(&m0init_lock);


	if (clovis_init_done && (pthread_self() != m0init_thread)) {
		printf("==========> tid=%d I am not the init thread\n",
		       (int)syscall(SYS_gettid));

		/* reminder: m0thread is stored in the TLS */
		M0_ALLOC_PTR(m0thread);
		if (m0thread == NULL)
			return -ENOMEM;

		memset(m0thread, 0, sizeof(struct m0_thread));

		m0_thread_adopt(m0thread, clovis_instance->m0c_motr);
	} else
		printf("----------> tid=%d I am the init thread\n",
		       (int)syscall(SYS_gettid));

	pthread_mutex_unlock(&m0init_lock);

	my_init_done = true;

	return 0;
}

int m0_obj_id_sscanf(char *idstr, struct m0_uint128 *obj_id)
{
	int rc;
	char  tmpfid[MAXNAMLEN];

	if (strchr(idstr, ':') == NULL) {
		obj_id->u_lo = atoi(idstr);
		return 0;
	}

	rc = m0_fid_sscanf(idstr, (struct m0_fid *)obj_id);
	if (rc != 0)
		fprintf(stderr, "can't m0_fid_sscanf() %s, rc:%d", idstr, rc);

	m0_fid_print(tmpfid, MAXNAMLEN,
		     (struct m0_fid *)obj_id);
	printf("====> Object id is %s\n", tmpfid);

	return rc;
}

void m0fini(void)
{
	if (pthread_self() == m0init_thread) {
		/* Finalize Clovis instance */
		m0_idx_fini(&idx);
		m0_client_fini(clovis_instance, true);
	} else {
		m0_thread_shun();
		m0_free(m0thread);
	}
}

int m0kvs_get(char *k, size_t klen,
	       char *v, size_t *vlen)
{
	struct m0_bufvec	 key;
	struct m0_bufvec	 val;
	int rc;

	if (!my_init_done)
		m0kvs_reinit();

	rc = m0_bufvec_alloc(&key, 1, klen) ?:
	     m0_bufvec_empty_alloc(&val, 1);

	memcpy(key.ov_buf[0], k, klen);
	memset(v, 0, *vlen);

	rc = m0_op_kvs(M0_IC_GET, &key, &val);
	if (rc)
		goto out;

	*vlen = (size_t)val.ov_vec.v_count[0];
	memcpy(v, (char *)val.ov_buf[0], *vlen);

out:
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;
}

int m0kvs_set(char *k, size_t klen,
	       char *v, size_t vlen)
{
	struct m0_bufvec	 key;
	struct m0_bufvec	 val;
	int rc;

	if (!my_init_done)
		m0kvs_reinit();

	rc = m0_bufvec_alloc(&key, 1, klen) ?:
	     m0_bufvec_alloc(&val, 1, vlen);
	if (rc)
		goto out;

	memcpy(key.ov_buf[0], k, klen);
	memcpy(val.ov_buf[0], v, vlen);

	rc = m0_op_kvs(M0_IC_PUT, &key, &val);
out:
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;
}

int m0kvs_del(char *k, size_t klen)
{
	struct m0_bufvec	 key;
	int rc;

	if (!my_init_done)
		m0kvs_reinit();

	rc = buf2vec(k, klen, &key);
	if (rc)
		goto out;

	rc = m0_op_kvs(M0_IC_DEL, &key, NULL);

out:
	free_buf2vec(&key);
	return rc;
}


int m0_pattern_kvs(char *k, char *pattern,
		   get_list_cb cb, void *arg_cb)
{
	struct m0_bufvec	   keys;
	struct m0_bufvec	   vals;
	struct m0_op       *op = NULL;
	int i = 0;
	int rc;
	int rcs[1];
	bool stop = false;
	char myk[KLEN];
	bool startp = false;
	int size = 0;
	int flags;

	strcpy(myk, k);
	flags = 0; /* Only for 1st iteration */

	do {
		/* Iterate over all records in the index. */
		rc = m0_bufvec_alloc(&keys, 1, KLEN) ?:
		     m0_bufvec_alloc(&vals, 1, VLEN);
		if (rc != 0)
			return rc;

		keys.ov_buf[0] = m0_alloc(strnlen(myk, KLEN)+1);
		keys.ov_vec.v_count[0] = strnlen(myk, KLEN)+1;
		strcpy(keys.ov_buf[0], myk);

		rc = m0_idx_op(&idx, M0_IC_NEXT, &keys, &vals,
				      rcs, flags,  &op);
		if (rc != 0) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);
			return rc;
		}
		pthread_mutex_lock(&big_motr_lock);
		m0_op_launch(&op, 1);
		pthread_mutex_unlock(&big_motr_lock);
		rc = m0_op_wait(op, M0_BITS(M0_OS_STABLE),
				       M0_TIME_NEVER);
		/* @todo : Why is op null after this call ??? */

		if (rc != 0) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);
#if 0
			if (op) {
				m0_op_fini(op);
				m0_free0(&op);
			}
#endif
			return rc;
		}

		if (rcs[0] == -ENOENT) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);
#if 0
			if (op) {
				m0_op_fini(op);
				m0_free0(&op);
			}
#endif

			/* No more keys to be found */
			if (startp)
				return 0;
			return -ENOENT;
		}
		for (i = 0; i < keys.ov_vec.v_nr; i++) {
			if (keys.ov_buf[i] == NULL) {
				stop = true;
				break;
			}

			/* Small state machine to display things
			 * (they are sorted) */
			if (!fnmatch(pattern, (char *)keys.ov_buf[i], 0)) {

				/* Avoid last one and use it as first
				 *  of next pass */
				if (!stop) {
					if (!cb((char *)keys.ov_buf[i],
						  arg_cb))
						break;
				}
				if (startp == false)
					startp = true;
			} else {
				if (startp == true) {
					stop = true;
					break;
				}
			}

			strcpy(myk, (char *)keys.ov_buf[i]);
			flags = M0_OIF_EXCLUDE_START_KEY;
		}

		m0_bufvec_free(&keys);
		m0_bufvec_free(&vals);
#if 0
		if (op) {
			m0_op_fini(op);
			m0_free0(&op);
		}
#endif

	} while (!stop);

	return size;
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

static int open_entity(struct m0_entity *entity)
{
	int		  rc;
	struct m0_op	*ops[1] = {NULL};

	rc = m0_entity_open(entity, &ops[0]);
	if (rc != 0)
		goto cleanup;

	m0_op_launch(ops, 1);
	rc = m0_op_wait(ops[0], M0_BITS(M0_OS_FAILED,
					       M0_OS_STABLE),
			       M0_TIME_NEVER);
	if (rc == 0)
		rc = ops[0]->op_rc;

cleanup:
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	ops[0] = NULL;

	return rc;
}

static int init_ctx(struct clovis_io_ctx *ioctx, off_t off,
		    int block_count, int block_size)
{
	int	     rc;
	int	     i;
	uint64_t	   last_index;

	/* Allocate block_count * 4K data buffer. */
	rc = m0_bufvec_alloc(&ioctx->data, block_count, block_size);
	if (rc != 0)
		return rc;

	/* Allocate bufvec and indexvec for write. */
	rc = m0_bufvec_alloc(&ioctx->attr, block_count, 1);
	if (rc != 0)
		return rc;

	rc = m0_indexvec_alloc(&ioctx->ext, block_count);
	if (rc != 0)
		return rc;

	last_index = off;
	for (i = 0; i < block_count; i++) {
		ioctx->ext.iv_index[i] = last_index;
		ioctx->ext.iv_vec.v_count[i] = block_size;
		last_index += block_size;

		/* we don't want any attributes */
		ioctx->attr.ov_vec.v_count[i] = 0;
	}

	return 0;
}


int m0store_create_object(struct m0_uint128 id)
{
	int		  rc;
	struct m0_obj obj;
	struct m0_op *ops[1] = {NULL};

	if (!my_init_done)
		m0kvs_reinit();

	memset(&obj, 0, sizeof(struct m0_obj));

	m0_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_client_layout_id(clovis_instance));

	m0_entity_create(NULL, &obj.ob_entity, &ops[0]);

	pthread_mutex_lock(&big_motr_lock);
	m0_op_launch(ops, ARRAY_SIZE(ops));
	pthread_mutex_unlock(&big_motr_lock);

	rc = m0_op_wait(
		ops[0], M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
		M0_TIME_NEVER);

	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	m0_entity_fini(&obj.ob_entity);

	return rc;
}

int m0store_delete_object(struct m0_uint128 id)
{
	int		  rc;
	struct m0_obj obj;
	struct m0_op *ops[1] = {NULL};

	if (!my_init_done)
		m0kvs_reinit();

	memset(&obj, 0, sizeof(struct m0_obj));

	m0_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_client_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	m0_entity_delete(&obj.ob_entity, &ops[0]);

	pthread_mutex_lock(&big_motr_lock);
	m0_op_launch(ops, ARRAY_SIZE(ops));
	pthread_mutex_unlock(&big_motr_lock);

	rc = m0_op_wait(
		ops[0], M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
		M0_TIME_NEVER);

	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	m0_entity_fini(&obj.ob_entity);

	return rc;
}


static int write_data_aligned(struct m0_uint128 id, char *buff, off_t off,
				int block_count, int block_size)
{
	int		  rc;
	int		  op_rc;
	int		  i;
	int		  nr_tries = 10;
	struct m0_obj obj;
	struct m0_op *ops[1] = {NULL};
	struct clovis_io_ctx    ioctx;

	if (!my_init_done)
		m0kvs_reinit();

again:
	init_ctx(&ioctx, off, block_count, block_size);

	for (i = 0; i < block_count; i++)
		memcpy(ioctx.data.ov_buf[i],
		       (char *)(buff+i*block_size),
		       block_size);

	/* Set the  bject entity we want to write */
	memset(&obj, 0, sizeof(struct m0_obj));

	m0_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_client_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	/* Create the write request */
	m0_obj_op(&obj, M0_OC_WRITE,
			 &ioctx.ext, &ioctx.data, &ioctx.attr, 0, 0, &ops[0]);

	/* Launch the write request*/
	pthread_mutex_lock(&big_motr_lock);
	m0_op_launch(ops, 1);
	pthread_mutex_unlock(&big_motr_lock);

	/* wait */
	rc = m0_op_wait(ops[0],
			       M0_BITS(M0_OS_FAILED,
			       M0_OS_STABLE),
			       M0_TIME_NEVER);
	op_rc = ops[0]->op_sm.sm_rc;

	/* fini and release */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	m0_entity_fini(&obj.ob_entity);

	if (op_rc == -EINVAL && nr_tries != 0) {
		nr_tries--;
		ops[0] = NULL;
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
	int		     i;
	int		     rc;
	struct m0_op    *ops[1] = {NULL};
	struct m0_obj    obj;
	uint64_t		last_index;
	struct clovis_io_ctx ioctx;

	if (!my_init_done)
		m0kvs_reinit();

	rc = m0_indexvec_alloc(&ioctx.ext, block_count);
	if (rc != 0)
		return rc;

	rc = m0_bufvec_alloc(&ioctx.data, block_count, block_size);
	if (rc != 0)
		return rc;
	rc = m0_bufvec_alloc(&ioctx.attr, block_count, 1);
	if (rc != 0)
		return rc;

	last_index = off;
	for (i = 0; i < block_count; i++) {
		ioctx.ext.iv_index[i] = last_index;
		ioctx.ext.iv_vec.v_count[i] = block_size;
		last_index += block_size;

		ioctx.attr.ov_vec.v_count[i] = 0;
	}

	/* Read the requisite number of blocks from the entity */
	memset(&obj, 0, sizeof(struct m0_obj));

	m0_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_client_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	/* Create the read request */
	m0_obj_op(&obj, M0_OC_READ,
			 &ioctx.ext, &ioctx.data, &ioctx.attr,
			 0, 0, &ops[0]);
	assert(rc == 0);
	assert(ops[0] != NULL);
	assert(ops[0]->op_sm.sm_rc == 0);

	pthread_mutex_lock(&big_motr_lock);
	m0_op_launch(ops, 1);
	pthread_mutex_unlock(&big_motr_lock);

	/* wait */
	rc = m0_op_wait(ops[0],
			       M0_BITS(M0_OS_FAILED,
			       M0_OS_STABLE),
			       M0_TIME_NEVER);
	assert(rc == 0);
	assert(ops[0]->op_sm.sm_state == M0_OS_STABLE);
	assert(ops[0]->op_sm.sm_rc == 0);

	for (i = 0; i < block_count; i++)
		memcpy((char *)(buff + block_size*i),
		       (char *)ioctx.data.ov_buf[i],
		       ioctx.data.ov_vec.v_count[i]);


	/* fini and release */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);
	m0_entity_fini(&obj.ob_entity);

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
 *     Lx		  Ux
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
	char *tmpbuff;

	tmpbuff = malloc(bs);
	if (tmpbuff == NULL)
		return -ENOMEM;

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
	 *	   x <--------------- len ------------------>
	 *  ---|-----x1-----|------------|------------|-------x2--|----
	 *     Lx1	  Ux1		       Lx2	 Ux2
	 *
	 * We should (write case)
	 *   1) read block [Lx1, Ux1] and update range [x1, Ux1]
	 *     then write updated [Lx1, Ux1]
	 *   3) read block [Lx2, Ux2], update [Lx2, x2] and
	 *       then writes back updated [Lx2, Ux2]
	 */
#if 0
	printf("IO: (%lld, %llu) = [%lld, %lld]\n",
		(long long)x, (unsigned long long)len,
		(long long)x1, (long long)x2);

	printf("  Bornes: %lld < %lld < %lld ||| %lld < %lld < %lld\n",
		(long long)Lx1, (long long)x1, (long long)Ux1,
		(long long)Lx2, (long long)x2, (long long)Ux2);
#endif
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
			*		 Lio
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
			*		x2
			*		Uio
			*		Ubond
			*/
			Uio = x2;
			bnext = false;
			Ubond = x2;
		} else {
			/* Not aligned on the left
			* --|---------x2--|----
			*   Uio
			*		 Ubond
			*/
			Uio = Lx2;
			bnext = true;
			Ubond = Ux2;
		}
	}

	/* delta_pos is the offset position in input buffer "buff"
	 * What is before buff+delta_pos has already been done */
	delta_pos = 0;
	if (bprev) {
		/* Reads block [Lx1, Ux1] before aligned [Lio, Uio] */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx1, 1, bs);
		if (rc < 0 || rc != bs) {
			free(tmpbuff);
			return -1;
		}

		/* Update content of read block
		 * --|-----------------------x1-----------|---
		 *   Lx1				  Ux1
		 *			      WORK HERE
		 *    <----------------------><---------->
		 *	  x1-Lx1		Ux1-x1
		 */
		delta_tmp = x1 - Lx1;
		switch (iotype) {
		case IO_WRITE:
			memcpy((char *)(tmpbuff+delta_tmp),
			       buff, (Ux1 - x1));

			/* Writes block [Lx1, Ux1] once updated */
			rc = write_data_aligned(id, tmpbuff, Lx1, 1, bs);
			if (rc < 0 || rc != bs) {
				free(tmpbuff);
				return -1;
			}

			break;

		case IO_READ:
			 memcpy(buff, (char *)(tmpbuff+delta_tmp),
			       (Ux1 - x1));
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		delta_pos += Ux1 - x1;
		done += Ux1 - x1;
	}

	if (Lio != Uio) {
		/* Easy case: aligned IO on aligned limit [Lio, Uio] */
		/* If no aligned block were found, then Uio == Lio */
		bcount = (Uio - Lio)/bs;
		switch (iotype) {
		case IO_WRITE:
			rc = write_data_aligned(id, (char *)(buff + delta_pos),
						Lio, bcount, bs);

			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		case IO_READ:
			rc = read_data_aligned(id, (char *)(buff + delta_pos),
					       Lio, bcount, bs);

			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		if (rc != (bcount*bs)) {
			free(tmpbuff);
			return -1;
		}

		done += rc;
		delta_pos += done;
	}

	if (bnext) {
		/* Reads block [Lx2, Ux2] after aligned [Lio, Uio] */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx2, 1, bs);
		if (rc < 0) {
			free(tmpbuff);
			return -1;
		}

		/* Update content of read block
		 * --|---------------x2------------------|---
		 *   Lx2				 Ux2
		 *       WORK HERE
		 *    <--------------><------------------>
		 *	  x2-Lx2	   Ux2-x2
		 */
		switch (iotype) {
		case IO_WRITE:
			memcpy(tmpbuff, (char *)(buff + delta_pos),
			      (x2 - Lx2));

			/* Writes block [Lx2, Ux2] once updated */
			/* /!\ This writes extraenous ending zeros */
			rc = write_data_aligned(id, tmpbuff, Lx2, 1, bs);
			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		case IO_READ:
			memcpy((char *)(buff + delta_pos), tmpbuff,
			       (x2 - Lx2));
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		done += x2 - Lx2;
	}

	if (insider) {
		/* Insider case read/update/write */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx1, 1, bs);
		if (rc < 0) {
			free(tmpbuff);
			return -1;
		}

		/* --|----------x1---------x2------------|---
		 *   Lx1=Lx2			     Ux1=Ux2
		 *		  UPDATE
		 *    <---------><---------->
		 *       x1-Lx1      x2-x1
		 */
		delta_tmp = x1 - Lx1;
		switch (iotype) {
		case IO_WRITE:
			memcpy((char *)(tmpbuff+delta_tmp), buff,
			       (x2 - x1));

			/* /!\ This writes extraenous ending zeros */
			rc = write_data_aligned(id, tmpbuff, Lx1, 1, bs);
			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		case IO_READ:
			memcpy(buff, (char *)(tmpbuff+delta_tmp),
			       (x2 - x1));
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		done += x2 - x1;
	}
#if 0
	printf("Complete IO : [%lld, %lld] => [%lld, %lld]\n",
		(long long)x1, (long long)x2,
		(long long)Lbond, (long long)Ubond);

	printf("End of IO : len=%llu  done=%lld\n\n",
	       (long long)len, (long long)done);
#endif
	free(tmpbuff);
	return done;
}

ssize_t m0store_get_bsize(struct m0_uint128 id)
{
	return m0_obj_layout_id_to_unit_size(
			m0_client_layout_id(clovis_instance));
}

/**
 * New stuff for BLK IO 
 **/
static inline uint32_t entity_sm_state(struct m0_obj *obj)
{
	return obj->ob_entity.en_sm.sm_state;
}

static int write_data_to_file(int fd, struct m0_bufvec *data)
{
	int i;
	int rc;
	int nr_blocks;

	nr_blocks = data->ov_vec.v_nr;
	for (i = 0; i < nr_blocks; ++i) {
		rc = write(fd, data->ov_buf[i], data->ov_vec.v_count[i]);
		if (rc != data->ov_vec.v_count[i])
			break;
	}

	return i;
}

static int read_data_from_file(int fd, struct m0_bufvec *data)
{
	int i;
	int rc;
	int nr_blocks;

	nr_blocks = data->ov_vec.v_nr;
	for (i = 0; i < nr_blocks; ++i) {
		rc = read(fd, data->ov_buf[i], data->ov_vec.v_count[i]);
		if (rc != data->ov_vec.v_count[i])
			break;
	}

	return i;
}

static void prepare_ext_vecs(struct m0_indexvec *ext,
			     struct m0_bufvec *attr,
			     uint32_t block_count, uint32_t block_size,
			     uint64_t *last_index)
{
	int      i;

	for (i = 0; i < block_count; ++i) {
		ext->iv_index[i] = *last_index;
		ext->iv_vec.v_count[i] = block_size;
		*last_index += block_size;

		/* we don't want any attributes */
		attr->ov_vec.v_count[i] = 0;
	}
}

static int alloc_vecs(struct m0_indexvec *ext, struct m0_bufvec *data,
		      struct m0_bufvec *attr, uint32_t block_count,
		      uint32_t block_size)
{
	int      rc;

	rc = m0_indexvec_alloc(ext, block_count);
	if (rc != 0)
		return rc;

	/*
 *	  * this allocates <block_count> * <block_size>  buffers for data,
 *		   * and initialises the bufvec for us.
 *			    */

	rc = m0_bufvec_alloc(data, block_count, block_size);
	if (rc != 0) {
		m0_indexvec_free(ext);
		return rc;
	}
	rc = m0_bufvec_alloc(attr, block_count, 1);
	if (rc != 0) {
		m0_indexvec_free(ext);
		m0_bufvec_free(data);
		return rc;
	}
	return rc;
}

static void cleanup_vecs(struct m0_bufvec *data, struct m0_bufvec *attr,
				struct m0_indexvec *ext)
{
		/* Free bufvec's and indexvec's */
		m0_indexvec_free(ext);
		m0_bufvec_free(data);
		m0_bufvec_free(attr);
}


static int read_data_from_object(struct m0_obj *obj,
				 struct m0_indexvec *ext,
				 struct m0_bufvec *data,
				 struct m0_bufvec *attr,
				 uint32_t flags)
{
	int		  rc;
	int		  op_rc;
	int		  nr_tries = 10;
	struct m0_op	*ops[1] = {NULL};

again:

	/* Create the read request */
	m0_obj_op(obj, M0_OC_READ, ext, data, attr, 0, flags, &ops[0]);
	if (ops[0] == NULL)
		return -EINVAL;
	M0_ASSERT(ops[0]->op_sm.sm_rc == 0);

	/* Launch the read request*/
	m0_op_launch(ops, 1);

	/* wait */
	rc = m0_op_wait(ops[0], M0_BITS(M0_OS_FAILED,
					       M0_OS_STABLE),
			       M0_TIME_NEVER);
	op_rc = ops[0]->op_sm.sm_rc;


	/* fini and release */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);

	if (op_rc == -EINVAL && nr_tries != 0) {
		nr_tries--;
		ops[0] = NULL;
		sleep(5);
		goto again;
	}

	return rc;
}

static int write_data_to_object(struct m0_obj *obj,
				struct m0_indexvec *ext,
				struct m0_bufvec *data,
				struct m0_bufvec *attr)
{
	int		  rc;
	int		  op_rc;
	int		  nr_tries = 10;
	struct m0_op	*ops[1] = {NULL};

again:

	/* Create the write request */
	m0_obj_op(obj, M0_OC_WRITE, ext, data, attr, 0, 0, &ops[0]);
	if (ops[0] == NULL)
		return -EINVAL;

	/* Launch the write request*/
	m0_op_launch(ops, 1);

	/* wait */
	rc = m0_op_wait(ops[0],
			M0_BITS(M0_OS_FAILED,
				M0_OS_STABLE),
			M0_TIME_NEVER);
	op_rc = ops[0]->op_sm.sm_rc;

	/* fini and release */
	m0_op_fini(ops[0]);
	m0_op_free(ops[0]);

	if (op_rc == -EINVAL && nr_tries != 0) {
		nr_tries--;
		ops[0] = NULL;
		sleep(5);
		goto again;
	}

	return rc;
}

int m0_write_bulk(int fd_src,
		  struct m0_uint128 id,
		  uint32_t block_size,
		  uint32_t block_count,
		  uint64_t update_offset,
		  int blks_per_io)
{
	int			   rc;
	struct m0_indexvec	    ext;
	struct m0_bufvec	      data;
	struct m0_bufvec	      attr;
	uint32_t		      bcount;
	uint64_t		      last_index;
	struct m0_obj		 obj;

	M0_SET0(&obj);
        m0_obj_init(&obj, &clovis_uber_realm, &id,
                           m0_client_layout_id(clovis_instance));

	rc = open_entity(&obj.ob_entity);

	if (rc)
		goto init_error;

	if (entity_sm_state(&obj) != M0_ES_OPEN || rc != 0)
		goto cleanup;

	last_index = update_offset;

	if (blks_per_io == 0)
		blks_per_io = M0_MAX_BLOCK_COUNT;

	rc = alloc_vecs(&ext, &data, &attr, blks_per_io, block_size);
	if (rc != 0)
		goto cleanup;

	while (block_count > 0) {
		bcount = (block_count > blks_per_io)?
			  blks_per_io:block_count;
		if (bcount < blks_per_io) {
			cleanup_vecs(&data, &attr, &ext);
			rc = alloc_vecs(&ext, &data, &attr, bcount,
					block_size);
			if (rc != 0)
				goto cleanup;
		}
		prepare_ext_vecs(&ext, &attr, bcount,
				 block_size, &last_index);

		/* Read data from source file. */
		rc = read_data_from_file(fd_src, &data);
		M0_ASSERT(rc == bcount);

		/* Copy data to the object*/
		rc = write_data_to_object(&obj, &ext, &data, &attr);
		if (rc != 0) {
			fprintf(stderr, "Writing to object failed!\n");
			cleanup_vecs(&data, &attr, &ext);
			goto cleanup;
		}
		block_count -= bcount;
	}
cleanup:
	cleanup_vecs(&data, &attr, &ext);
	/* fini and release */
init_error:
	m0_entity_fini(&obj.ob_entity);
	return rc;
}

int m0_read_bulk(int fd_dest,
		 struct m0_uint128 id,
		 uint32_t block_size,
		 uint32_t block_count,
		 uint64_t update_offset,
		 int blks_per_io,
	    	 uint32_t flags)
{
	int			   rc;
	uint64_t		      last_index = 0;
	struct m0_obj		 obj;
	struct m0_indexvec	    ext;
	struct m0_bufvec	      data;
	struct m0_bufvec	      attr;
	uint32_t		      bcount;

	/* Read the requisite number of blocks from the entity */
	M0_SET0(&obj);
        m0_obj_init(&obj, &clovis_uber_realm, &id,
                           m0_client_layout_id(clovis_instance));

	rc = open_entity(&obj.ob_entity);
	if (entity_sm_state(&obj) != M0_ES_OPEN || rc != 0)
		goto cleanup;

	/***** ICI ****/
	last_index = update_offset;

       if (blks_per_io == 0)
		blks_per_io = M0_MAX_BLOCK_COUNT;

	rc = alloc_vecs(&ext, &data, &attr, blks_per_io, block_size);
	if (rc != 0)
		goto cleanup;

	while (block_count > 0) {
		bcount = (block_count > blks_per_io)?
			  blks_per_io:block_count;
		if (bcount < blks_per_io) {
			cleanup_vecs(&data, &attr, &ext);
			rc = alloc_vecs(&ext, &data, &attr, bcount,
					block_size);
			if (rc != 0)
				goto cleanup;
		}
		prepare_ext_vecs(&ext, &attr, bcount,
				 block_size, &last_index);

		rc = read_data_from_object(&obj, &ext, &data, &attr, flags);
		if (rc != 0) {
			fprintf(stderr, "Reading from object failed!\n");
			cleanup_vecs(&data, &attr, &ext);
			goto cleanup;
		}

		rc = write_data_to_file(fd_dest, &data);
		M0_ASSERT(rc == bcount);

		block_count -= bcount;
	}

	cleanup_vecs(&data, &attr, &ext);

cleanup:
	m0_entity_fini(&obj.ob_entity);
	return rc;
}


