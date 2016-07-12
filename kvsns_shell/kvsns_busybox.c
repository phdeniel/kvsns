#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
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
	kvsns_ino_t ino;

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
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		kvshl_set_char(k, v);

		strcpy(k, "KVSNS_PARENT_INODE");
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		kvshl_set_char(k, v);

		strcpy(k, "KVSNS_PATH");
		strcpy(v, "/");
		kvshl_set_char(k, v);

		strcpy(k, "KVSNS_PREV_PATH");
		strcpy(v, "/");
		kvshl_set_char(k, v);
	} else if (!strcmp(exec_name, "ns_init")) {
		rc = kvsns_init_root();
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}
	} else if (!strcmp(exec_name, "ns_creat")) {
		if (argc != 2) {
			fprintf(stderr, "creat <newfile>\n");
			exit(1);
		}
		rc = kvsns_creat(&cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n", 
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "ns_ln")) {
		if (argc != 3) {
			fprintf(stderr, "ln <newdir> <content>\n");
			exit(1);
		}
		rc = kvsns_symlink(&cred, &current_inode, argv[1],
				   argv[2], &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n", 
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "ns_readlink")) {
		char link_content[MAXPATHLEN];
		int size = MAXPATHLEN;
		if (argc != 2) {
			fprintf(stderr, "readlink <link>\n");
			exit(1);
		}

		rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
		if (rc != 0) {
			printf("==> %llun not accessible = %llu : rc=%d\n", 
				current_inode, ino, rc);
			return 0;
		}

		rc = kvsns_readlink(&cred, &ino,
				    link_content, &size);
		if (rc == 0) {
			printf("==> %llu/%s %llu content = %s\n", 
				current_inode, argv[1], ino, link_content);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);

	} else if (!strcmp(exec_name, "ns_mkdir")) {
		if (argc != 2) {
			fprintf(stderr, "mkdir <newdir>\n");
			exit(1);
		}
		rc = kvsns_mkdir(&cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n", 
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "ns_rmdir")) {
		if (argc != 2) {
			fprintf(stderr, "rmdir <newdir>\n");
			exit(1);
		}
		rc = kvsns_rmdir(&cred, &current_inode, argv[1]);
		if (rc == 0) {
			printf("==> %llu/%s deleted = %llu\n", 
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "ns_cd")) {
		if (argc != 2) {
			fprintf(stderr, "cd <dir>\n");
			exit(1);
		}
		if (!strcmp(argv[1], ".")) {
			if (rc != 0) {
				printf("==> %llu parent not accessible = %llu : rc=%d\n", 
					current_inode, rc);
				return 0;
			}
			printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
			exec_name, current_inode, parent_inode,
			current_path, prev_path);

			return 0;
		} else if (!strcmp(argv[1], "..")) {
			rc = kvsns_lookupp(&cred, &current_inode, &ino);
			if (rc != 0) {
				printf("==> %llun not accessible = %llu : rc=%d\n", 
					current_inode, ino, rc);
				return 0;
			}

			strcpy(k, "KVSNS_PARENT_INODE");
			snprintf(v, VLEN, "%llu", current_inode);
			parent_inode = current_inode; 
			kvshl_set_char(k, v);

			strcpy(k, "KVSNS_INODE");
			snprintf(v, VLEN, "%llu", ino);
			current_inode = ino;
			kvshl_set_char(k, v);

			strcpy(k, "KVSNS_PREV_PATH");
			strcpy(current_path, prev_path);
			strcpy(v, prev_path);
			kvshl_set_char(k, v);

			strcpy(k, "KVSNS_PATH");
			strcpy(v, current_path);
			kvshl_set_char(k, v);

			printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
				exec_name, current_inode, parent_inode,
			current_path, prev_path);

		} else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0) {
				printf("==> %llu/%s not accessible = %llu : rc=%d\n", 
					current_inode, argv[1], ino, rc);
				return 0;
			}
			strcpy(k, "KVSNS_PARENT_INODE");
			snprintf(v, VLEN, "%llu", current_inode);
			parent_inode = current_inode; 
			kvshl_set_char(k, v);

			strcpy(k, "KVSNS_INODE");
			snprintf(v, VLEN, "%llu", ino);
			current_inode = ino;
			kvshl_set_char(k, v);

			strcpy(k, "KVSNS_PREV_PATH");
			strcpy(prev_path, current_path);
			strcpy(v, prev_path);
			kvshl_set_char(k, v);

			strcpy(k, "KVSNS_PATH");
			snprintf(current_path, MAXPATHLEN, "%/%s", 
				v, argv[1]);
			strcpy(v, current_path);
			kvshl_set_char(k, v);

			printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
				exec_name, current_inode, parent_inode,
			current_path, prev_path);

			return 0;
		}
	} else if (!strcmp(exec_name, "ns_ls")) {
		int offset;
		int size;
		kvsns_dentry_t dirent[10];
		int i;

		offset = 0;
		size = 10;
		do {
			rc = kvsns_readdir(&cred, &current_inode, offset, 
				           dirent, &size);
			if (rc != 0) {
				printf("==> readdir failed rc=%d\n", rc);
				exit(1);
			}
			printf( "===> size = %d\n", size);
			for (i=0; i < size; i++)
				printf("%d %s/%s = %llu\n", 
					offset+i, current_path, dirent[i].name,
					dirent[i].inode);

			if (size == 0)
				break;

			offset += size ;
			size = 10;
		} while (1);
	} else if (!strcmp(exec_name, "ns_getattr")) {
		struct stat buffstat;
		rc = kvsns_getattr(&cred, &current_inode, &buffstat);
		if (rc == 0) {
 			printf(" inode: %ld\n",                         buffstat.st_ino);
			printf(" protection: %o\n",                     buffstat.st_mode);
			printf(" number of hard links: %d\n",           buffstat.st_nlink);
			printf(" user ID of owner: %d\n",               buffstat.st_uid);
			printf(" group ID of owner: %d\n",              buffstat.st_gid);
			printf(" total size, in bytes: %ld\n",          buffstat.st_size);
			printf(" blocksize for filesystem I/O: %ld\n",  buffstat.st_blksize);
			printf(" number of blocks allocated: %ld\n",    buffstat.st_blocks);
			printf(" time of last access: %ld : %s",        buffstat.st_atime, 
				ctime(&buffstat.st_atime));
			printf(" time of last modification: %ld : %s",  buffstat.st_mtime,
				 ctime(&buffstat.st_mtime));
			printf(" time of last change: %ld : %s",        buffstat.st_ctime,
				 ctime(&buffstat.st_ctime));

			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
		return 0;
	} else if (!strcmp(exec_name, "ns_rm")) {
	} else
		fprintf(stderr, "%s does not exists\n", exec_name);
	printf("######## OK ########\n");
	return 0;
}
