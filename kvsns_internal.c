#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "kvsns.h"
#include <string.h>

int kvsns_next_inode(kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -EINVAL;

	rc = kvshl_incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str)
{
	char *token;
	char *rest = str;
	int maxsize = *size;
	int pos = 0;

	if (!inolist || !str || !size)
		return -EINVAL;

	while((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%llu", &inolist[pos++]);

		if (pos == maxsize)
			break;
	}

	*size = pos;

	return 0;
}

int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str)
{
	int i;
	char tmp[VLEN];

	if (!inolist || !str)
		return -EINVAL;

	strcpy(str, "");

	for (i=0; i < size ; i++)
		if (inolist[i] != 0LL) {
			snprintf(tmp, VLEN, "%llu|", inolist[i]);
			strcat(str, tmp);
		}
	
	return 0;	
}
