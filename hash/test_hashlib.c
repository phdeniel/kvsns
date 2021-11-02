#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "hashlib.h"

int main(int argc, char *argv[]) {
	int rc = 0;
	hashbuff128_t hb;

	memset(hb, 0, sizeof(hashbuff128_t));

	rc = hashlib_murmur3_128("grp","item", hb);	
	printf("====> rc=%d hb=%"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32"\n",
		rc, hb[0], hb[1], hb[2], hb[3]);

	return 0;
}
