#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "../kvsal/kvsal.h"

int main(int argc, char *argv[])
{
	int rc;
	int i;
	char key[KLEN];
	char val[VLEN];

	if (argc != 3) {
		fprintf(stderr, "3 args\n");
		exit(1);
	}

	rc = kvsns_start();
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(-rc);
	}

	strncpy(key, argv[1], KLEN);
	strncpy(val, argv[2], VLEN);
	rc = kvsal_set_char(key, val);
	if (rc != 0) {
		fprintf(stderr, "kvsal_set_char: err=%d\n", rc);
		exit(-rc);
	}

	printf("+++++++++++++++\n");

	exit(0);
	return 0;
}
