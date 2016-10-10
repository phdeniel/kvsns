#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "kvsns.h"

#define SIZE 1024

int main(int argc, char *argv[])
{
	int rc;
	int i;
	int end;
	kvsns_ino_t ino;
	kvsns_ino_t parent;
	struct stat bufstat;
	char key[KLEN];
	char val[VLEN];
	char tmp[VLEN];
	kvsns_file_open_t fd;
	kvsns_cred_t cred;
	ssize_t written;
	ssize_t read;
	char buff[SIZE];
	char buff2[SIZE];
	size_t count;
	off_t offset;

	cred.uid = getuid();
	cred.gid = getgid();

	printf("uid=%u gid=%u, pid=%d\n",
		getuid(), getgid(), getpid());

	rc = kvsns_start();
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(1);
	}

	rc = kvsns_init_root(1);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
		exit(1);
	}

	parent = KVSNS_ROOT_INODE;
	rc = kvsns_creat(&cred, &parent, "fichier", 0755, &ino);
	if (rc != 0) {
		if (rc == -EEXIST)
			fprintf(stderr, "dirent exists\n");
		else {
			fprintf(stderr, "kvsns_creat: err=%d\n", rc);
			exit(1);
		}
	}

	rc = kvsns_open(&cred, &ino, 0 /* to raise errors laters */,
			 0755, &fd);
	if (rc != 0) {
		fprintf(stderr, "kvsns_open: err=%d\n", rc);
		exit(1);
	}

	strncpy(buff, "This is the content of the file", SIZE);
	count = strnlen(buff, SIZE);
	offset = 0;

	written = kvsns_write(&cred, &fd, buff, count, offset);
	if (written < 0) {
		fprintf(stderr, "kvsns_write: err=%lld\n", written);
		exit(1);
	}

	memset(buff2, 0, SIZE);

	offset = 2;
	read = kvsns_read(&cred, &fd, buff2, SIZE, offset);
	if (read < 0) {
		fprintf(stderr, "kvsns_read: err=%lld\n", read);
		exit(1);
	}

	printf("Buff= #%s#\n", buff);
	printf("Buff2= #%s# at offset %lld\n", buff2, offset);

	rc = kvsns_unlink(&cred, &parent, "fichier");
	if (rc != 0) {
		fprintf(stderr, "kvsns_unlink: err=%d\n", rc);
		exit(1);
	}

	rc = kvsns_close(&fd);
	if (rc != 0) {
		fprintf(stderr, "kvsns_close: err=%d\n", rc);
		exit(1);
	}


	printf("######## OK ########\n");
	return 0;

}
