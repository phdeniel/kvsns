#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>

int main(int argc, char *argv[])
{
	int rc;
	int i;
	int howmany;
	char key[KLEN];
	char val[VLEN];

	if (argc != 3) {
		fprintf(stderr, "pattern how_many args\n");
		exit(1);
	}

	howmany = atoi(argv[2]);

	rc = kvsal_init();
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}

	rc = kvsal_begin_transaction();
	if (rc != 0) {
		fprintf(stderr, "kvsal_begin_transaction: err=%d\n", rc);
		exit(-rc);
	}

	for (i = 0; i < howmany; i++) {
		snprintf(key, KLEN, "%s%d", argv[1], i);
		rc = kvsal_del(key);
		if (rc != 0) {
			fprintf(stderr, "kvsal_del: err=%d\n", rc);
			exit(-rc);
		}
	}

	rc = kvsal_end_transaction();
	if (rc != 0) {
		fprintf(stderr, "kvsal_end_transaction: err=%d\n", rc);
		exit(-rc);
	}

	rc = kvsal_fini();
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}
	printf("+++++++++++++++\n");

	exit(0);
	return 0;
}
