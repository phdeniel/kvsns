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

int copy_to_mero(int fd_source, struct m0_uint128 id,
		 int iolen, size_t filesize)
{
	size_t rsize, wsize;
	size_t bsize;
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

	rc = m0_write_bulk(fd_source,
	 		   id,
			   bsize, 
			   nb, 
			   0, 
			   0);
	printf("===> rc=%d\n", rc);

	rsize = pread(fd_source, buff, remain, aligned_offset);
	if (rsize < 0) {
                        free(buff);
                        return -1;
        }

	wsize = m0store_pwrite(id, aligned_offset, rsize, bsize, buff);
        if (wsize < 0) {
                free(buff);
                return -1;
        }

        if (wsize != rsize) {
                free(buff);
                return -1;
        }

	/* Think about setting MD */
	free(buff);
	return 0;
}

int main(int argc, char *argv[])
{
	int fds;
	struct m0_uint128 id;
	struct stat stat;
	int iolen;
	int rc;
	struct collection_item *errors = NULL;

	if (argc != 4) {
		fprintf(stderr, "cp <src> <id> <iolen>\n");
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
	if (m0_obj_id_sscanf(argv[2], &id) < 0) {
		fprintf(stderr, "Bad Id\n");
		exit(1);
	}

	if (!entity_id_is_valid(&id)) {
		fprintf(stderr, "Entity Id is invalid\n");
		exit(1);
	}

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

	m0fini();

	return 0;
}
