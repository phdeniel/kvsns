#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "../kvsal/kvsal.h"

#define LIST_TRUNK 10

int main(int argc, char *argv[])
{
	int rc;
	int i;
	char key[KLEN];
	int offset = 0;
	int size = LIST_TRUNK;
	int maxsize = 0;
	kvsal_item_t items[LIST_TRUNK];
	kvsal_list_t list;

	if (argc != 2) {
		fprintf(stderr, "1 args\n");
		exit(1);
	}

	rc = kvsal_init();
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}

	snprintf(key, KLEN, "%s*", argv[1]);
	rc = kvsal_get_list_size(key);
	if (rc < 0) {
		fprintf(stderr, "kvsal_get_list_size: err=%d\n", rc);
		exit(1);
	}
	maxsize = rc;
	printf("kvsal_get_list_size: found %d items\n", rc);

	rc = kvsal_fetch_list(key, &list);
	if (rc != 0) {
		fprintf(stderr, "kvsal_fetch_list: err=%d\n", rc);
		exit(-rc);
	}

	while (offset < maxsize) {
		size = LIST_TRUNK;
		rc = kvsal_get_list2(&list, offset, &size, items);
		if (rc < 0) {
			fprintf(stderr, "kvsal_get_list2: err=%d\n", rc);
			exit(-rc);
		}
		for (i = 0; i < size ; i++)
			printf("==> %d %s\n", offset+i, items[i].str);

		offset += size;
	}

	rc = kvsal_dispose_list(&list);
	if (rc != 0) {
		fprintf(stderr, "kvsal_fetch_list: err=%d\n", rc);
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
