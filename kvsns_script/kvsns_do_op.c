/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <libgen.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

extern struct kvsal_ops kvsal;

extern kvsns_cred_t cred;

extern char current_path[MAXPATHLEN];
extern char prev_path[MAXPATHLEN];
extern char exec_name[MAXPATHLEN];
extern char k[KLEN];
extern char v[VLEN];

extern kvsns_ino_t current_inode;
extern kvsns_ino_t parent_inode;
extern kvsns_ino_t ino;

int do_op(int argc, char *argv[])
{
	int rc;

	strncpy(exec_name, basename(argv[0]), MAXPATHLEN);

	printf("+++> '%s'\n", exec_name);

	if (exec_name[0] == '#')
		return 0;

	/* Look at exec name and see what is to be done */
	if (!strcmp(exec_name, "reset")) {
		strncpy(k, "KVSNS_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		kvsal.set_char(k, v);

		strncpy(k, "KVSNS_PARENT_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		kvsal.set_char(k, v);

		strncpy(k, "KVSNS_PATH", KLEN);
		strncpy(v, "/", VLEN);
		kvsal.set_char(k, v);

		strncpy(k, "KVSNS_PREV_PATH", KLEN);
		strncpy(v, "/", VLEN);
		kvsal.set_char(k, v);
	} else if (!strcmp(exec_name, "init")) {
		rc = kvsns_init_root(1); /* Open Bar */
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			return 1;
		}

		strncpy(k, "KVSNS_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		rc = kvsal.set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			return 1;
		}

		strncpy(k, "KVSNS_PARENT_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		rc = kvsal.set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			return 1;
		}

		strncpy(k, "KVSNS_PATH", KLEN);
		strncpy(v, "/", VLEN);
		rc = kvsal.set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			return 1;
		}

		strncpy(k, "KVSNS_PREV_PATH", KLEN);
		strncpy(v, "/", VLEN);
		rc = kvsal.set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			return 1;
		}
	} else if (!strcmp(exec_name, "creat")) {
		if (argc != 2) {
			fprintf(stderr, "creat <newfile>\n");
			return 1;
		}
		rc = kvsns_creat(&cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "ln")) {
		if (argc != 3) {
			fprintf(stderr, "ln <newdir> <content>\n");
			return 1;
		}
		rc = kvsns_symlink(&cred, &current_inode, argv[1],
				   argv[2], &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "readlink")) {
		char link_content[MAXPATHLEN];
		size_t size = MAXPATHLEN;

		if (argc != 2) {
			fprintf(stderr, "readlink <link>\n");
			return 1;
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

	} else if (!strcmp(exec_name, "mkdir")) {
		if (argc != 2) {
			fprintf(stderr, "mkdir <newdir>\n");
			return 1;
		}
		rc = kvsns_mkdir(&cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "rmdir")) {
		if (argc != 2) {
			fprintf(stderr, "rmdir <newdir>\n");
			return 1;
		}
		rc = kvsns_rmdir(&cred, &current_inode, argv[1]);
		if (rc == 0) {
			printf("==> %llu/%s deleted = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "cd")) {
		if (argc != 2) {
			fprintf(stderr, "cd <dir>\n");
			return 1;
		}
		if (!strcmp(argv[1], ".")) {
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

			strncpy(k, "KVSNS_PARENT_INODE", KLEN);
			snprintf(v, VLEN, "%llu", current_inode);
			parent_inode = current_inode;
			kvsal.set_char(k, v);

			strncpy(k, "KVSNS_INODE", KLEN);
			snprintf(v, VLEN, "%llu", ino);
			current_inode = ino;
			kvsal.set_char(k, v);

			strncpy(k, "KVSNS_PREV_PATH", KLEN);
			strncpy(current_path, prev_path, MAXPATHLEN);
			strncpy(v, prev_path, VLEN);
			kvsal.set_char(k, v);

			strncpy(k, "KVSNS_PATH", KLEN);
			strncpy(v, current_path, VLEN);
			kvsal.set_char(k, v);

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
			strncpy(k, "KVSNS_PARENT_INODE", KLEN);
			snprintf(v, VLEN, "%llu", current_inode);
			parent_inode = current_inode;
			kvsal.set_char(k, v);

			strncpy(k, "KVSNS_INODE", KLEN);
			snprintf(v, VLEN, "%llu", ino);
			current_inode = ino;
			kvsal.set_char(k, v);

			strncpy(k, "KVSNS_PREV_PATH", KLEN);
			strncpy(prev_path, current_path, MAXPATHLEN);
			strncpy(v, prev_path, VLEN);
			kvsal.set_char(k, v);

			strncpy(k, "KVSNS_PATH", KLEN);
			snprintf(current_path, MAXPATHLEN, "%s/%s",
				v, argv[1]);
			strncpy(v, current_path, VLEN);
			kvsal.set_char(k, v);

			printf("exec=%s ino=%llu parent=%llu path=%s prev=%s\n",
				exec_name, current_inode, parent_inode,
			current_path, prev_path);

			return 0;
		}
	} else if (!strcmp(exec_name, "ls")) {
		off_t offset;
		int size;
		kvsns_dentry_t dirent[10];
		kvsns_dir_t dirfd;
		int i;
	memset(dirent, 0, 10*sizeof(kvsns_dentry_t));

		rc = kvsns_opendir(&cred, &current_inode, &dirfd);
		if (rc != 0) {
			printf("==> opendir failed rc=%d\n", rc);
			return 1;
		}

		offset = 0;
		do {
			size = 10;
			rc = kvsns_readdir(&cred, &dirfd, offset,
					   dirent, &size);
			if (rc != 0) {
				printf("==> readdir failed rc=%d\n", rc);
				return 1;
			}
			printf("===> size = %d\n", size);
			for (i = 0; i < size; i++)
				printf("%lld %s/%s = %llu\n",
					(long long)(offset+i), current_path,
					dirent[i].name,
					dirent[i].inode);

			offset += size;
		} while (size != 0);

		rc = kvsns_closedir(&dirfd);
		if (rc != 0) {
			printf("==> closedir failed rc=%d\n", rc);
			return 1;
		}

	} else if (!strcmp(exec_name, "getattr")) {
		struct stat buffstat;

		if (argc != 2) {
			fprintf(stderr, "getattr <name>\n");
			return 1;
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
			       (int)buffstat.st_nlink);
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
	} else if (!strcmp(exec_name, "archive")) {
		if (argc != 2) {
			fprintf(stderr, "archive <name>\n");
			return 1;
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_archive(&cred, &ino);
		if (rc == 0)
			printf("Success !!\n");
		else
			printf("Archive failed, rc=%d\n", rc);

		return 0;

	} else if (!strcmp(exec_name, "restore")) {
		if (argc != 2) {
			fprintf(stderr, "restore <name>\n");
			return 1;
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_restore(&cred, &ino);
		if (rc == 0)
			printf("Success !!\n");
		else
			printf("Archive failed, rc=%d\n", rc);

		return 0;

	} else if (!strcmp(exec_name, "release")) {
		if (argc != 2) {
			fprintf(stderr, "release <name>\n");
			return 1;
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_release(&cred, &ino);
		if (rc == 0)
			printf("Success !!\n");
		else
			printf("Release failed, rc=%d\n", rc);

		return 0;

	} else if (!strcmp(exec_name, "state")) {
		char state[VLEN];

		if (argc != 2) {
			fprintf(stderr, "state <name>\n");
			return 1;
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns_lookup(&cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_state(&cred, &ino, state);
		if (rc == 0)
			printf("Success !! state=%s\n", state);
		else
			printf("state failed, rc=%d\n", rc);

		return 0;

	} else if (!strcmp(exec_name, "truncate")) {
		kvsns_ino_t fino = 0LL;
		struct stat stat;
		memset(&stat, 0, sizeof(stat));

		if (argc != 3) {
			fprintf(stderr, "truncate <file> <offset>\n");
			return 1;
		}

		rc = kvsns_lookup(&cred, &current_inode, argv[1], &fino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			return 1;
		}

		stat.st_size = atoi(argv[2]);

		rc = kvsns_setattr(&cred, &fino, &stat, STAT_SIZE_SET);
		if (rc == 0)
			printf("Truncate %llu/%s %d OK\n", current_inode,
			       argv[1], atoi(argv[2]));
		else
			fprintf(stderr, "Can't unlink %llu/%s rc=%d\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "setxattr")) {
		if (argc != 4) {
			printf("sexattr name xattr_name xattr_val\n");
			return 1;
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
			printf("setxattr: %llu --> %s = %s CREATED\n",
				ino, argv[2], argv[3]);
		else
			fprintf(stderr, "Failed : %d\n", rc);
	 } else if (!strcmp(exec_name, "getxattr")) {
		char value[VLEN];
		size_t xattr_size = KLEN;

		if (argc != 3) {
			printf("gexattr name xattr_name\n");
			return 1;
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
			printf("getxattr: %llu --> %s = %s\n",
				ino, argv[2], value);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "removexattr")) {
		if (argc != 3) {
			printf("removexattr name\n");
			return 1;
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
			printf("removexattr: %llu --> %s DELETED\n",
				ino, argv[2]);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "listxattr")) {
		int offset;
		int size;
		int i = 0;
		kvsns_xattr_t dirent[10];

		if (argc != 2) {
			printf("listxattr name\n");
			return 1;
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
				return 1;
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
	} else if (!strcmp(exec_name, "clearxattr")) {
		if (argc != 2) {
			printf("clearxattr name\n");
			return 1;
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

	} else if (!strcmp(exec_name, "link")) {
		kvsns_ino_t dino = 0LL;
		kvsns_ino_t sino = 0LL;
		char dname[MAXNAMLEN];

		if (argc != 3 && argc != 4) {
			printf("link srcname newname (same dir)\n");
			printf("link srcname dstdir newname\n");
			return 1;
		}

		rc = kvsns_lookup(&cred, &current_inode, argv[1], &sino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			return 1;
		}

		if (argc == 3) {
			dino = current_inode;
			strncpy(dname, argv[2], MAXNAMLEN);
		} else if (argc == 4) {
			rc = kvsns_lookup(&cred, &current_inode,
				  argv[2], &dino);
			if (rc != 0) {
				fprintf(stderr, "%s/%s does not exist\n",
					current_path, argv[1]);
				return 1;
			}
			strncpy(dname, argv[3], MAXNAMLEN);
		}
		printf("%s: sino=%llu dino=%llu dname=%s\n",
			exec_name, sino, dino, dname);

		rc = kvsns_link(&cred, &sino, &dino, dname);
		if (rc == 0)
			printf("link: %llu --> %llu/%s CREATED\n",
				sino, dino, dname);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "rm")) {
		if (argc != 2) {
			fprintf(stderr, "unlink <newdir>\n");
			return 1;
		}

		rc = kvsns_unlink(&cred, &current_inode, argv[1]);
		if (rc == 0)
			printf("Unlink %llu/%s OK\n", current_inode, argv[1]);
		else
			fprintf(stderr, "Can't unlink %llu/%s rc=%d\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "rename")) {
		kvsns_ino_t sino = 0LL;
		kvsns_ino_t dino = 0LL;

		if (argc != 5) {
			fprintf(stderr, "rename sdir sname ddir dname\n");
			return 1;
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
				return 1;
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
				return 1;
			}
		}

		rc = kvsns_rename(&cred, &sino, argv[2], &dino, argv[4]);
		if (rc == 0)
			printf("Rename %llu/%s => %llu/%s OK\n",
				sino, argv[2], dino, argv[4]);
		else
			fprintf(stderr, "%llu/%s => %llu/%s Failed rc=%d\n",
				sino, argv[2], dino, argv[4], rc);
	} else if (!strcmp(exec_name, "fsstat")) {
		kvsns_fsstat_t statfs;

		rc = kvsns_fsstat(&statfs);
		if (rc != 0) {
			fprintf(stderr, "kvsns_statfs failed rc=%d\n", rc);
			return 1;
		}
		printf("FSSTAT: nb_inodes = %u\n",
			(unsigned int)statfs.nb_inodes);
	} else if (!strcmp(exec_name, "mr_proper")) {
		rc = kvsns_mr_proper();
		printf("Mr Proper: rc=%d\n", rc);
	} else if (!strcmp(exec_name, "print")) {
		int i;

		for (i = 0; i < argc; i++)
			printf("%s ", argv[i]);
		printf("\n");
	} else
		fprintf(stderr, "%s does not exists\n", exec_name);

	printf("######## OK ########\n");
	return 0;
}
