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
#include "../../mero/m0common.h"

static struct collection_item *cfg_items;
#define CONF "~/kvsns.ini"

int copy_from_mero(struct m0_uint128 id, int fd_dest,
		   int iolen, size_t filesize)
{
	off_t off;
	size_t rsize, wsize;
	int rc;
	int remains;
	size_t len;
	ssize_t bsize;
	char *buff;

	bsize = m0store_get_bsize(id);
	if (bsize < 0)
		return bsize;

	buff = malloc(bsize*M0STORE_BLK_COUNT);
	if (buff == NULL)
		return -ENOMEM;

	remains = filesize;
	off = 0LL;
	while (off < filesize) {
		len = (remains > iolen) ? iolen : remains;

		rsize = m0store_pread(id, off, len, bsize, buff);
		if (rsize < 0) {
			free(buff);
			return -1;
		}

		wsize = pwrite(fd_dest, buff, rsize, off);
		if (wsize < 0) {
			free(buff);
			return -1;
		}

		if (wsize != rsize) {
			free(buff);
			return -1;
		}

		off += rsize;
		remains -= rsize;
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

	id = M0_CLOVIS_ID_APP;
	id.u_lo = atoi(argv[1]);

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
