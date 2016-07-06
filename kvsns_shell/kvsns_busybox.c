#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <libgen.h>
#include "../kvsns.h"

int main(int argc, char *argv[])
{
	int rc;
	kvsns_cred_t cred;

	char current_path[MAXPATHLEN];
	char prev_path[MAXPATHLEN];
	char exec_name[MAXPATHLEN];
	char k[KLEN];
	char v[VLEN];

	kvsns_ino_t current_inode;
	kvsns_ino_t parent_inode;

	cred.uid = getuid();
	cred.gid = getgid();

	strncpy(exec_name, basename(argv[0]), MAXPATHLEN);

	rc = kvsns_start();
	if (rc != 0) {
		fprintf(stderr, "kvsns_start: err=%d\n", rc);
		exit(1);
	}

	strcpy(k, "KVSNS_INODE");	
	if (kvshl_get_char(k, v) == 0) 	
		sscanf(v, "%llu", &current_inode);

	strcpy(k, "KVSNS_PARENT_INODE");	
	if (kvshl_get_char(k, v) == 0) 	
		sscanf(v, "%llu", &parent_inode);

	strcpy(k, "KVSNS_PATH");	
	if (kvshl_get_char(k, v) == 0) 	
		strcpy(current_path, v);

	strcpy(k, "KVSNS_PREV_PATH");	
	if (kvshl_get_char(k, v) == 0) 	
		strcpy(prev_path, v);

	printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
		exec_name, current_inode, parent_inode,
		current_path, prev_path);

	/* Look at exec name and see what is to be done */
	if (!strcmp(exec_name, "ns_reset")) {
		strcpy(k, "KVSNS_INODE");
		strcpy(v, "2");
		kvshl_set_char(k, v);

		strcpy(k, "KVSNS_PARENT_INODE");
		strcpy(v, "2");
		kvshl_set_char(k, v);

		strcpy(k, "KVSNS_PATH");
		strcpy(v, "/");
		kvshl_set_char(k, v);

		strcpy(k, "KVSNS_PREV_PATH");
		strcpy(v, "/");
		kvshl_set_char(k, v);
	} else if (!strcmp(exec_name, "ns_mkdir")) {
	} else if (!strcmp(exec_name, "ns_rmdir")) {
	} else if (!strcmp(exec_name, "ns_cd")) {
	} else if (!strcmp(exec_name, "ns_getattr")) {
	} 
	printf("######## OK ########\n");
	return 0;
}
