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
#include "../kvsal/kvsal.h"
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
	if (kvsal_get_char(k, v) == 0)
		sscanf(v, "%llu", &current_inode);

	strcpy(k, "KVSNS_PARENT_INODE");
	if (kvsal_get_char(k, v) == 0)
		sscanf(v, "%llu", &parent_inode);

	strcpy(k, "KVSNS_PATH");
	if (kvsal_get_char(k, v) == 0)
		strcpy(current_path, v);

	strcpy(k, "KVSNS_PREV_PATH");
	if (kvsal_get_char(k, v) == 0)
		strcpy(prev_path, v);

	printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
		exec_name, current_inode, parent_inode,
		current_path, prev_path);

	/* Look at exec name and see what is to be done */
	if (!strcmp(exec_name, "ns_reset")) {
		strcpy(k, "KVSNS_INODE");
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		kvsal_set_char(k, v);

		strcpy(k, "KVSNS_PARENT_INODE");
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		kvsal_set_char(k, v);

		strcpy(k, "KVSNS_PATH");
		strcpy(v, "/");
		kvsal_set_char(k, v);

		strcpy(k, "KVSNS_PREV_PATH");
		strcpy(v, "/");
		kvsal_set_char(k, v);
	} else if (!strcmp(exec_name, "ns_init")) {
		rc = kvsns_init_root(1); /* Open Bar */
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strcpy(k, "KVSNS_INODE");
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strcpy(k, "KVSNS_PARENT_INODE");
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strcpy(k, "KVSNS_PATH");
		strcpy(v, "/");
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strcpy(k, "KVSNS_PREV_PATH");
		strcpy(v, "/");
		rc = kvsal_set_char(k, v);
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
		size_t size = MAXPATHLEN;

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
				printf("%llu unaccessible %llu rc=%d\n",
					current_inode, rc);
				return 0;
			}
			printf("exec=%s ino=%llu parent=%llu path=%s prev=%s\n",
			exec_name, current_inode, parent_inode,
			current_path, prev_path);

			return 0;
		} else if (!strcmp(argv[1], "..")) {
			rc = kvsns_lookupp(&cred, &current_inode, &ino);
			if (rc != 0) {
				printf("%llun not accessible = %llu : rc=%d\n",
					current_inode, ino, rc);
				return 0;
			}

			strcpy(k, "KVSNS_PARENT_INODE");
			snprintf(v, VLEN, "%llu", current_inode);
			parent_inode = current_inode;
			kvsal_set_char(k, v);

			strcpy(k, "KVSNS_INODE");
			snprintf(v, VLEN, "%llu", ino);
			current_inode = ino;
			kvsal_set_char(k, v);

			strcpy(k, "KVSNS_PREV_PATH");
			strcpy(current_path, prev_path);
			strcpy(v, prev_path);
			kvsal_set_char(k, v);

			strcpy(k, "KVSNS_PATH");
			strcpy(v, current_path);
			kvsal_set_char(k, v);

			printf("exec=%s ino=%llu parent=%llu path=%s prev=%s\n",
				exec_name, current_inode, parent_inode,
			current_path, prev_path);

		} else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0) {
				printf("%llu/%s not accessible = %llu: rc=%d\n",
					current_inode, argv[1], ino, rc);
				return 0;
			}
			strcpy(k, "KVSNS_PARENT_INODE");
			snprintf(v, VLEN, "%llu", current_inode);
			parent_inode = current_inode;
			kvsal_set_char(k, v);

			strcpy(k, "KVSNS_INODE");
			snprintf(v, VLEN, "%llu", ino);
			current_inode = ino;
			kvsal_set_char(k, v);

			strcpy(k, "KVSNS_PREV_PATH");
			strcpy(prev_path, current_path);
			strcpy(v, prev_path);
			kvsal_set_char(k, v);

			strcpy(k, "KVSNS_PATH");
			snprintf(current_path, MAXPATHLEN, "%/%s",
				v, argv[1]);
			strcpy(v, current_path);
			kvsal_set_char(k, v);

			printf("exec=%s ino=%llu parent=%llu path=%s prev=%s\n",
				exec_name, current_inode, parent_inode,
			current_path, prev_path);

			return 0;
		}
	} else if (!strcmp(exec_name, "ns_ls")) {
		off_t offset;
		int size;
		kvsns_dentry_t dirent[10];
		kvsns_dir_t dirfd;
		int i;

		rc = kvsns_opendir(&cred, &current_inode, &dirfd);
		if (rc != 0) {
			printf("==> opendir failed rc=%d\n", rc);
			exit(1);
		}

		offset = 0;
		do {
			size = 10;
			rc = kvsns_readdir(&cred, &dirfd, offset,
					   dirent, &size);
			if (rc != 0) {
				printf("==> readdir failed rc=%d\n", rc);
				exit(1);
			}
			printf("===> size = %d\n", size);
			for (i = 0; i < size; i++)
				printf("%d %s/%s = %llu\n",
					offset+i, current_path, dirent[i].name,
					dirent[i].inode);

			offset += size;
		} while (size != 0);

		rc = kvsns_closedir(&dirfd);
		if (rc != 0) {
			printf("==> closedir failed rc=%d\n", rc);
			exit(1);
		}

	} else if (!strcmp(exec_name, "ns_getattr")) {
		struct stat buffstat;

		if (argc != 2) {
			fprintf(stderr, "getattr <name>\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_getattr(&cred, &ino, &buffstat);
		if (rc == 0) {
			printf(" inode: %ld\n", buffstat.st_ino);
			printf(" mode: %o\n", buffstat.st_mode);
			printf(" number of hard links: %d\n",
			       buffstat.st_nlink);
			printf(" user ID of owner: %d\n", buffstat.st_uid);
			printf(" group ID of owner: %d\n", buffstat.st_gid);
			printf(" total size, in bytes: %ld\n",
			       buffstat.st_size);
			printf(" blocksize for filesystem I/O: %ld\n",
				buffstat.st_blksize);
			printf(" number of blocks allocated: %ld\n",
				buffstat.st_blocks);
			printf(" time of last access: %ld : %s",
				buffstat.st_atime,
				ctime(&buffstat.st_atime));
			printf(" time of last modification: %ld : %s",
				 buffstat.st_mtime,
				 ctime(&buffstat.st_mtime));
			printf(" time of last change: %ld : %s",
				buffstat.st_ctime,
				 ctime(&buffstat.st_ctime));

			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
		return 0;
	} else if (!strcmp(exec_name, "ns_truncate")) {
		kvsns_ino_t fino;
		struct stat stat;

		if (argc != 3) {
			fprintf(stderr, "truncate <file> <offset>\n");
			exit(1);
		}

		rc = kvsns_lookup(&cred, &current_inode, argv[1], &fino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			exit(1);
		}

		stat.st_size = atoi(argv[2]);

		rc = kvsns_setattr(&cred, &fino, &stat, STAT_SIZE_SET);
		if (rc == 0)
			printf("Truncate %llu/%s %d OK\n", current_inode,
			       argv[1], atoi(argv[2]));
		else
			fprintf(stderr, "Can't unlink %llu/%s rc=%d\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "ns_setxattr")) {
		if (argc != 4) {
			printf("ns_sexattr name xattr_name xattr_val\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_setxattr(&cred, &ino, argv[2],
				    argv[3], strlen(argv[3])+1, 0);
		if (rc == 0)
			printf("ns_setxattr: %llu --> %s = %s CREATED\n",
				ino, argv[2], argv[3]);
		else
			fprintf(stderr, "Failed : %d\n", rc);
	 } else if (!strcmp(exec_name, "ns_getxattr")) {
		char value[VLEN];
		size_t xattr_size = KLEN;

		if (argc != 3) {
			printf("ns_gexattr name xattr_name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_getxattr(&cred, &ino, argv[2], value, &xattr_size);
		if (rc == 0)
			printf("ns_getxattr: %llu --> %s = %s\n",
				ino, argv[2], value);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "ns_removexattr")) {
		if (argc != 3) {
			printf("ns_removexattr name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}
		rc = kvsns_removexattr(&cred, &ino, argv[2]);
		if (rc == 0)
			printf("ns_removexattr: %llu --> %s DELETED\n",
				ino, argv[2]);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "ns_listxattr")) {
		int offset;
		int size;
		int i = 0;
		kvsns_xattr_t dirent[10];

		if (argc != 2) {
			printf("ns_listxattr name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		offset = 0;
		size = 10;
		do {
			rc = kvsns_listxattr(&cred, &ino, offset,
					     dirent, &size);
			if (rc != 0) {
				printf("==> readdir failed rc=%d\n", rc);
				exit(1);
			}
			printf("===> size = %d\n", size);
			for (i = 0; i < size; i++)
				printf("%d %s -> xattr:%s\n",
					offset+i, current_path, dirent[i].name);

			if (size == 0)
				break;

			offset += size;
			size = 10;
		} while (1);
	} else if (!strcmp(exec_name, "ns_clearxattr")) {
		if (argc != 2) {
			printf("ns_clearxattr name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_remove_all_xattr(&cred, &ino);
		if (rc == 0)
			printf("All xattr deleted for %s = %llu\n",
			       argv[1], ino);
		else
			fprintf(stderr, "Failed !!!\n");

	} else if (!strcmp(exec_name, "ns_link")) {
		kvsns_ino_t dino;
		kvsns_ino_t sino;
		char dname[MAXNAMLEN];

		if (argc != 3 && argc != 4) {
			printf("ns_link srcname newname (same dir)\n");
			printf("ns_link srcname dstdir newname\n");
			exit(1);
		}

		rc = kvsns_lookup(&cred, &current_inode, argv[1], &sino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			exit(1);
		}

		if (argc == 3) {
			dino = current_inode;
			strcpy(dname, argv[2]);
		} else if (argc == 4) {
			rc = kvsns_lookup(&cred, &current_inode,
				  argv[2], &dino);
			if (rc != 0) {
				fprintf(stderr, "%s/%s does not exist\n",
					current_path, argv[1]);
				exit(1);
			}
			strcpy(dname, argv[3]);
		}
		printf("%s: sino=%llu dino=%llu dname=%s\n",
			exec_name, sino, dino, dname);

		rc = kvsns_link(&cred, &sino, &dino, dname);
		if (rc == 0)
			printf("ns_link: %llu --> %llu/%s CREATED\n",
				sino, dino, dname);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "ns_rm")) {
		if (argc != 2) {
			fprintf(stderr, "unlink <newdir>\n");
			exit(1);
		}

		rc = kvsns_unlink(&cred, &current_inode, argv[1]);
		if (rc == 0)
			printf("Unlink %llu/%s OK\n", current_inode, argv[1]);
		else
			fprintf(stderr, "Can't unlink %llu/%s rc=%d\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "ns_rename")) {
		kvsns_ino_t sino;
		kvsns_ino_t dino;

		if (argc != 5) {
			fprintf(stderr, "rename sdir sname ddir dname\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			sino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode,
					  argv[2], &sino);
			if (rc != 0) {
				fprintf(stderr,
					"%llu/%s does not exist rc=%d\n",
					current_inode, argv[2], rc);
				exit(1);
			}
		}

		if (!strcmp(argv[3], "."))
			dino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode,
					  argv[4], &dino);
			if (rc != 0) {
				fprintf(stderr,
					"%llu/%s does not exist rc=%d\n",
					current_inode, argv[2], rc);
				exit(1);
			}
		}

		rc = kvsns_rename(&cred, &sino, argv[2], &dino, argv[4]);
		if (rc == 0)
			printf("Rename %llu/%s => %llu/%s OK\n",
				sino, argv[2], dino, argv[4]);
		else
			fprintf(stderr, "%llu/%s => %llu/%s Failed rc=%d\n",
				sino, argv[2], dino, argv[4], rc);
	} else if (!strcmp(exec_name, "ns_fsstat")) {
		kvsns_fsstat_t statfs;

		rc = kvsns_fsstat(&statfs);
		printf("FSSTAT: nb_inodes = %llu\n", statfs.nb_inodes);
	} else if (!strcmp(exec_name, "ns_mr_proper")) {
		rc = kvsns_mr_proper();
		printf("Mr Proper: rc=%d\n", rc);
	} else
		fprintf(stderr, "%s does not exists\n", exec_name);
	printf("######## OK ########\n");
	return 0;
}
