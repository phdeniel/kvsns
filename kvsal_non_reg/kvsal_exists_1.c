#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>

int main(int argc, char *argv[])
{
	int rc;
	char key[KLEN];

	if (argc != 2) {
		fprintf(stderr, "2 args\n");
		exit(1);
	}

	rc = kvsal_init(NULL);
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}

	strncpy(key, argv[1], KLEN);
	rc = kvsal_exists(key);
	if (rc != 0) {
		fprintf(stderr, "kvsal_exists: err=%d\n", rc);
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
