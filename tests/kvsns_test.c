/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_test.c
 * KVSNS: simple test
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

/* Required Protoype */
int kvsns_next_inode(kvsns_ino_t *ino);

int main(int argc, char *argv[])
{
	int rc;
	int i;
	int end;
	kvsns_ino_t ino = 0LL;
	kvsns_ino_t ino2 = 0LL;
	kvsns_ino_t parent = 0LL;
	char val[VLEN];
	char tmp[VLEN];
	kvsns_cred_t cred;
	kvsal_item_t items[10];

	cred.uid = getuid();
	cred.gid = getgid();

	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(1);
	}

	rc = kvsns_init_root(1);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
		exit(1);
	}

	/* KVS FUNCTIONS */
	rc = kvsns_next_inode(&ino);
	if (rc != 0) {
		fprintf(stderr, "kvsns_next_inode: err=%d\n", rc);
		exit(1);
	}
	printf("kvsns_next_inode: ino=%llu\n", ino);

	rc = kvsal_set_char("test", "value");
	if (rc != 0) {
		fprintf(stderr, "kvsns_set_char: err=%d\n", rc);
		exit(1);
	}

	rc = kvsal_get_char("test", val);
	if (rc != 0) {
		fprintf(stderr, "kvsns_get_char: err=%d\n", rc);
		exit(1);
	}
	printf("kvsal_get_char: val=%s\n", val);

	rc = kvsal_exists("test");
	printf("Check existing key rc=%d\n", rc);

	rc = kvsal_exists("testfail");
	printf("Check non-existing key rc=%d\n", rc);

	rc = kvsal_del("test");
	if (rc != 0) {
		fprintf(stderr, "kvsns_get_char: err=%d\n", rc);
		exit(1);
	}
	printf("kvsal_get_char after del: %d\n", kvsal_get_char("test", val));

	rc = kvsal_get_list_size("*");
	if (rc < 0) {
		fprintf(stderr, "kvsal_get_list_size: err=%d\n", rc);
		exit(1);
	}
	printf("kvsal_get_list_size * : rc=%d\n", rc);

	end = 10;
	rc = kvsal_get_list_pattern("*", 0, &end, items);
	if (rc != 0) {
		fprintf(stderr, "kvsns_get_list_pattern: err=%d\n", rc);
		exit(1);
	}
	for (i = 0 ; i < end ; i++)
		printf("==> LIST: %s\n", items[i].str);

	printf("+++++++++++++++\n");
	end = 10;
	rc = kvsal_get_list_pattern("2.dentries.*", 0, &end, items);
	if (rc != 0) {
		fprintf(stderr, "kvsns_get_list_pattern: err=%d\n", rc);
		exit(1);
	}
	for (i = 0 ; i < end ; i++) {
		printf("==> LIST: %s\n", items[i].str);
		if (sscanf(items[i].str,
			   "%llu.dentries.%s",
			    &ino, tmp) == EOF)
			exit(1);
		printf("+++> LIST: %llu %s\n", ino, tmp);
	}
	printf("+++++++++++++++\n");


	/* NS FUNCTION */
	parent = KVSNS_ROOT_INODE;
	rc = kvsns_mkdir(&cred, &parent, "mydir", 0755, &ino);
	if (rc != 0) {
		if (rc == -EEXIST)
			printf("dirent exists\n");
		else {
			fprintf(stderr, "kvsns_mkdir: err=%d\n", rc);
			exit(1);
		}
	}
	printf("===> New Ino = %llu\n", ino);

	rc = kvsns_mkdir(&cred, &parent, "mydir2", 0755, &ino);
	if (rc != 0) {
		if (rc == -EEXIST)
			printf("dirent exists\n");
		else {
			fprintf(stderr, "kvsns_mkdir: err=%d\n", rc);
			exit(1);
		}
	}
	printf("===> New Ino = %llu\n", ino);

	rc = kvsns_lookup(&cred, &parent, "mydir", &ino);
	if (rc != 0) {
		fprintf(stderr, "kvsns_lookup: err=%d\n", rc);
		exit(1);
	}
	printf("====> INO LOOKUP = %llu\n", ino);

	rc = kvsns_lookupp(&cred, &ino, &ino2);
	if (rc != 0) {
		fprintf(stderr, "kvsns_lookupp: err=%d\n", rc);
		exit(1);
	}
	printf("====> INO LOOKUP = %llu|%llu\n", ino2, parent);
	if (parent != ino2)
		fprintf(stderr,
			"kvsns_lookupp: mauvaise implémentation !!!\n");

	rc = kvsns_rmdir(&cred, &parent, "mydir");
	if (rc != 0) {
		fprintf(stderr, "kvsns_rmdir: err=%d\n", rc);
		exit(1);
	}

	printf("######## THE END : OK ########\n");
	return 0;

}
