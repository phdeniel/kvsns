#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>


#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

#include "extstore.h"

void * test_create(void * arg)
{
	inogen_t ino;
	int rc;

	ino.inode = getpid();
	ino.generation = 0;

	rc = create_object(&ino);
	printf("test_create (pid=%d): rc=%d\n", getpid(), rc);

	return NULL;
}

void * test_delete(void * arg)
{
	inogen_t ino;
	int rc;

	ino.inode = getpid();
	ino.generation = 0;

	rc = delete_object(&ino);
	printf("test_delete (pid=%d): rc=%d\n", getpid(), rc);

	return NULL;
}

#define BUFFSIZE 4096

void * test_write(void * arg)
{
	inogen_t ino;
	int rc;
	char *datawrite;

	ino.inode = getpid();
	ino.generation = 0;

	datawrite = malloc(BUFFSIZE);
	if (!datawrite) {
		fprintf(stderr, "malloc error\n");
		exit(0);
	}
	datawrite[10] = 'z';
	datawrite[BUFFSIZE-10] = 'q';
	rc = pwrite_clovis(&ino, 0, BUFFSIZE, (char *)datawrite);
	printf("test_write (pid=%d): rc=%d\n", getpid(), rc);

	return NULL;
}

void * test_read(void * arg)
{
	inogen_t ino;
	int rc;
	char *dataread;

	ino.inode = getpid();
	ino.generation = 0;

	dataread = malloc(BUFFSIZE);
	if (!dataread) {
		fprintf(stderr, "malloc error\n");
		exit(0);
	}
	rc = pread_clovis(&ino, 0, BUFFSIZE, (char *)dataread);
	printf("====> %c\n", dataread[10]);
	printf("----> %c\n", dataread[BUFFSIZE-10]);
	printf("test_read (pid=%d): rc=%d\n", getpid(), rc);

	return NULL;
}

