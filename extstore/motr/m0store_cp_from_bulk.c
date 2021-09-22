#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ini_config.h>
#include "../../motr/m0common.h"

static struct collection_item *cfg_items;
#define CONF "/etc/kvsns.d/kvsns.ini"

int copy_from_mero(struct m0_uint128 id, int fd_dest,
		   int iolen, size_t filesize)
{
	size_t rsize, wsize;
	ssize_t bsize;
	char *buff;

	size_t remain;
	size_t aligned_offset;
	size_t nb;

	int rc;

	bsize = m0store_get_bsize(id);
	if (bsize < 0)
		return bsize;

	remain = filesize % bsize;
	nb = filesize / bsize;
	aligned_offset = nb * bsize;

	printf("filesize=%zd bsize=%zd nb=%zd remain=%zd aligned_offset=%zd\n",
		filesize, bsize, nb, remain, aligned_offset);

	buff = malloc(bsize);
	if (buff == NULL)
		return -ENOMEM;

	rc = m0_read_bulk(fd_dest,
			  id,
			  bsize,
			  nb,
			  0,
			  0,
			  0);
	printf("===> rc=%d\n", rc);

	rsize = m0store_pread(id, aligned_offset, remain, bsize, buff);
	if (rsize < 0) {
		free(buff);
		return -1;
	}

	wsize = pwrite(fd_dest, buff, rsize, aligned_offset);
	if (wsize < 0) {
		free(buff);
		return -1;
	}

	/* This POC writes on aligned blocks, we should align to real size */
	/* Useful in this case ?? */
	rc = ftruncate(fd_dest, filesize);
	if (rc < 0) {
		free(buff);
		return -1;
	}

	free(buff);
	return 0;
}

int main(int argc, char *argv[])
{
	int fdd;
	struct m0_uint128 id;
	int iolen;
	int rc;
	size_t filesize;
	struct collection_item *errors = NULL;

	if (argc != 5) {
		fprintf(stderr, "cp <id> <dest> <iolen> <size>\n");
		exit(1);
	}

	rc = config_from_file("libkvsns", CONF, &cfg_items,
				INI_STOP_ON_ERROR, &errors);
	if (rc) {
		fprintf(stderr, "Can't load config rc=%d\n", rc);

		free_ini_config_errors(errors);
		return -rc;
	}

	rc = m0init(cfg_items);
	if (rc != 0) {
		fprintf(stderr, "Can't start m0store lib, rc=%d\n", rc);
		exit(1);
	}

	iolen = atoi(argv[3]);
	printf("Start: iolen=%u\n", iolen);

	/* hande object id */
	if (m0_obj_id_sscanf(argv[1], &id) < 0) {
		fprintf(stderr, "Bad Id\n");
		exit(1);
	}

	if (!entity_id_is_valid(&id)) {
		fprintf(stderr, "Entity Id is invalid\n");
		exit(1);
	}


	filesize = atoi(argv[4]);

	/* Must read and write on fd */
	fdd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC);
	if (fdd < 0) {
		fprintf(stderr, "Can't open %s : %d\n",
			argv[2], errno);
		exit(1);
	}
	/* Must read and write on fd */
	if (copy_from_mero(id, fdd, iolen, filesize) < 0) {
		fprintf(stderr, "Can't copy to %s to MERO\n",
			argv[1]);
		exit(1);
	}

	close(fdd);

	m0fini();

	return 0;
}
