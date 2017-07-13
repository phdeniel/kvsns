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
#include "m0store.h"

int copy_from_mero(struct m0_uint128 id, int fd_dest,
		   int iolen, size_t filesize)
{
	off_t off;
	size_t rsize, wsize;
	int rc;
	int remains;
	size_t len;
	char buff[M0STORE_BLK_COUNT*M0STORE_BLK_SIZE];

	remains = filesize;
	off = 0LL;
	while (off < filesize) {
		len = (remains > iolen) ? iolen : remains;

		rsize = m0store_pread(id, off, len, M0STORE_BLK_SIZE, buff);
		if (rsize < 0)
			return -1;

		wsize = pwrite(fd_dest, buff, rsize, off);
		if (wsize < 0)
			return -1;

		if (wsize != rsize)
			return -1;

		off += rsize;
		remains -= rsize;
	}

	/* This POC writes on aligned blocks, we should align to real size */
	/* Useful in this case ?? */
	rc = ftruncate(fd_dest, filesize);
	if (rc < 0)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int fdd;
	struct m0_uint128 id;
	int iolen;
	int rc;
	size_t filesize;

	if (argc != 5) {
		fprintf(stderr, "cp <id> <dest> <iolen> <size>\n");
		exit(1);
	}

	rc = m0store_init(NULL);
	if (rc != 0) {
		fprintf(stderr, "Can't start m0store lib, rc=%d\n", rc);
		exit(1);
	}

	iolen = atoi(argv[3]);
	printf("Start: bs=%u iolen=%u\n", M0STORE_BLK_SIZE, iolen);

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

	m0store_fini();

	return 0;
}
