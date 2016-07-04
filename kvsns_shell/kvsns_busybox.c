#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "../kvsns.h"

int main(int argc, char *argv[])
{
	int rc;
	kvsns_ino_t ino;
	kvsns_ino_t ino2;
	kvsns_ino_t parent;
	struct stat bufstat;
	struct stat bufstat2;
	char key[KLEN];
	char val[VLEN];
	kvsns_cred_t cred;

	cred.uid = getuid();
	cred.gid = getgid();

	rc = kvsns_start();
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(1);
	}

	rc = kvsns_init_root();
	if (rc != 0) {
		fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
		exit(1);
	}

	printf("######## OK ########\n");
	return 0;

}
