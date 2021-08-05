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

kvsns_cred_t cred;

char current_path[MAXPATHLEN];
char prev_path[MAXPATHLEN];
char exec_name[MAXPATHLEN];
char k[KLEN];
char v[VLEN];

kvsns_ino_t current_inode = 0LL;
kvsns_ino_t parent_inode = 0LL;
kvsns_ino_t ino = 0LL;

/* This function is defined in the other file */
int do_op(int argc, char *argv[]);

static int setargs(char *args, char **argv)
{
	int count = 0;

	while (isspace(*args)) ++args;
	while (*args) {
		if (argv) argv[count] = args;
		while (*args && !isspace(*args)) ++args;
		if (argv && *args) *args++ = '\0';
		while (isspace(*args)) ++args;
		count++;
	}
	return count;
}

char **parsedargs(char *args, int *argc)
{
	char **argv = NULL;
	int    argn = 0;

	if (args && *args
		 && (args = strdup(args))
		 && (argn = setargs(args,NULL))
		 && (argv = malloc((argn+1) * sizeof(char *)))) {
		*argv++ = args;
		argn = setargs(args,argv);
	}

	if (args && !argv) free(args);

	*argc = argn;
	return argv;
}

void freeparsedargs(char **argv)
{
	if (argv) {
		free(argv[-1]);
		free(argv-1);
	} 
}

int read_file_by_line(char *filename)
{
	FILE *file;
	char line[256];

#ifdef DEBUG
	int i;
#endif
	char **av;
	int ac;

	int rc = 0;

	file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Can't open %s : errno=%d\n", filename, errno);
		exit(1);
	}
	
	while (fgets(line, sizeof(line), file)) {
		printf("= exec =>%s", line);

	
		av = parsedargs(line, &ac);
#ifdef DEBUG
		printf("\tac == %d\n",ac);
		for (i = 0; i < ac; i++)
			printf("\t\t[%s]\n",av[i]);
#endif
		if (ac != 0)
			rc = do_op(ac, av);

		freeparsedargs(av);

		if (rc)
			return rc;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int rc;

	cred.uid = getuid();
	cred.gid = getgid();

	strncpy(exec_name, basename(argv[0]), MAXPATHLEN);

	if (argc != 2) {
		fprintf(stderr, "%s <script file>\n", exec_name);
		exit(1); 
	}

	rc = kvsns_start(NULL);
	if (rc != 0) {
		fprintf(stderr, "kvsns_start: err=%d\n", rc);
		return 1;
	}

	strncpy(k, "KVSNS_INODE", KLEN);
	if (kvsal.get_char(k, v) == 0)
		sscanf(v, "%llu", &current_inode);

	strncpy(k, "KVSNS_PARENT_INODE", KLEN);
	if (kvsal.get_char(k, v) == 0)
		sscanf(v, "%llu", &parent_inode);

	strncpy(k, "KVSNS_PATH", KLEN);
	if (kvsal.get_char(k, v) == 0)
		strncpy(current_path, v, MAXPATHLEN);

	strncpy(k, "KVSNS_PREV_PATH", KLEN);
	if (kvsal.get_char(k, v) == 0)
		strncpy(prev_path, v, MAXPATHLEN);

	printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
		exec_name, current_inode, parent_inode,
		current_path, prev_path);

	return read_file_by_line(argv[1]);	
}


