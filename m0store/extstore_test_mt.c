#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"

#include "extstore.h"

char clovis_local_addr[] = "10.2.3.196@tcp:12345:44:101";
char clovis_ha_addr[] = "10.2.3.196@tcp:12345:45:1";
char clovis_confd_addr[] = "10.2.3.196@tcp:12345:44:101";
char clovis_prof[] = "<0x7000000000000001:0>";
char clovis_index_dir[] = "/tmp";

void * test_create(void *args);
void * test_delete(void *args);
void * test_write(void *args);
void * test_read(void *args);

int main(int argc, char *argv[])
{
	int rc;
	pthread_t thr;

	printf("Hello\n");

	rc = init_clovis(clovis_local_addr, 
			 clovis_ha_addr,
			 clovis_confd_addr,
			 clovis_prof,
			 clovis_index_dir);

	printf("init_clovis:rc=%d\n", rc);

	pthread_create(&thr, NULL, test_create, NULL);
	pthread_create(&thr, NULL, test_write, NULL);
	pthread_create(&thr, NULL, test_read, NULL);
	pthread_create(&thr, NULL, test_delete, NULL);

	fini_clovis();

	return 0;
}
