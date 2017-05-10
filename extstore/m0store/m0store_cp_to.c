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

int copy_to_mero(int fd_source, struct m0_uint128 id,
		 int iolen, size_t filesize)
{
	off_t off;
	size_t rsize, wsize;
	int remains;
	size_t len;
	char buff[BLK_COUNT*BLK_SIZE];

	remains = filesize;
	off = 0LL;
	while (off < filesize) {
		len = (remains > iolen) ? iolen : remains;

		rsize = pread(fd_source, buff, len, off);
		if (rsize < 0)
			return -1;

		wsize = m0_pwrite(id, off, rsize, BLK_SIZE, buff);
		if (wsize < 0)
			return -1;

		if (wsize != rsize)
			return -1;

		off += rsize;
		remains -= rsize;
	}

	/* Think about setting MD */
	return 0;
}

int main(int argc, char *argv[])
{
	int fds;
	struct m0_uint128 id;
	struct stat stat;
	int iolen;
	int rc;

	if (argc != 4) {
		fprintf(stderr, "cp <src> <id> <iolen>\n");
		exit(1);
	}

	rc = m0store_init();
	if (rc != 0) {
		fprintf(stderr, "Can't start m0store lib, rc=%d\n", rc);
		exit(1);
	}

	iolen = atoi(argv[3]);
	printf("Start: bs=%u iolen=%u\n", BLK_SIZE, iolen);

	id = M0_CLOVIS_ID_APP;
	id.u_lo = atoi(argv[2]);

	fds = open(argv[1], O_RDONLY);
	if (fds < 0) {
		fprintf(stderr, "Can't open %s : %d\n",
			argv[1], errno);
		exit(1);
	}

	if (fstat(fds, &stat) < 0) {
		fprintf(stderr, "Can't stat %s: %d\n", argv[1], errno);
		exit(1);
	}

	rc = m0store_create_object(id);
	if (rc != 0) {
		fprintf(stderr, "Can't create object rc=%d\n", rc);
		exit(1);
	}

	/* Must read and write on fd */
	if (copy_to_mero(fds, id, iolen, stat.st_size) < 0) {
		fprintf(stderr, "Can't copy to %s to MERO\n",
			argv[1]);
		exit(1);
	}

	close(fds);

	m0store_fini();

	return 0;
}
