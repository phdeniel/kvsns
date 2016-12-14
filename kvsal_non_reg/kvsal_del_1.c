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

	if (argc != 2) {
		fprintf(stderr, "2 args\n");
		exit(1);
	}

	rc = kvsns_start();
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(-rc);
	}

	strncpy(key, argv[1], KLEN);
	rc = kvsal_del(key);
	if (rc != 0) {
		fprintf(stderr, "kvsal_del: err=%d\n", rc);
		exit(-rc);
	}

	printf("+++++++++++++++\n");
	exit(0);
	return 0;
}
