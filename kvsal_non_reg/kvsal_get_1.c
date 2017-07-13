#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>

int main(int argc, char *argv[])
{
	int rc;
	int i;
	char key[KLEN];
	char val[VLEN];

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
	rc = kvsal_get_char(key, val);
	if (rc != 0) {
		fprintf(stderr, "kvsal_get_char: err=%d\n", rc);
		exit(-rc);
	}
	printf("key=%s val=%s\n", key, val);

	rc = kvsal_fini();
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}

	printf("+++++++++++++++\n");
	exit(0);
	return 0;
}
