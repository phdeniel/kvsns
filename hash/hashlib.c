/**
 * @file  hashlib.c
 * @brief Main file for sources of the HashLib API 
 */
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "murmur3.h"
#include "macaddr.h"
#include <kvsns/hashlib.h>

static int sprint_label(char *label, int labellen, char *grp, char *item)
{
	int len;
	mac_addr_t macaddr;
	int rc;

	rc = get_mac_addr(macaddr); 

	if (rc)
		len = snprintf(label, labellen, "%ld|%s|%s", time(NULL), grp, item);
	else
		len = snprintf(label, labellen, "%02x:%02x:%02x:%02x:%02x:%02x|%ld|%s|%s", 
				macaddr[0],  macaddr[1],  macaddr[2],  macaddr[3],
				macaddr[4],  macaddr[5], time(NULL), grp, item);	   
	return len;
}

/**
 * @brief Generate a hashed value on 128 bits using Murmur3
 *
 * This function computes a hashed value, using 2 strings as input.
 * This function does not pre-allocate the buffer used to store the
 * hashed value. It is the caller explicit responsability to allocate it
 *
 *
 * @param[in]    grp   	a string identifying the group of hash values
 * @param[in]    item	a string identifying an item inside a group 
 * @param[inout] hb 	preallocated buffer, will receive the hash
 *
 * @return 	0 means a success, negative value are errors
 * 		-return is a valid errno value when applicable
 */

int hashlib_murmur3_128(char *grp, char *item, hashbuff128_t hb) {
	char label[MAXSTRLEN];
	int len;

	/* Sanity check */
	if (!hb)
		return -EINVAL;

	len = sprint_label(label, MAXSTRLEN, grp, item);

	printf("===> label=#%s#\n", label);
	
	if (len < 0)
		return len;

	MurmurHash3_x64_128(label,len, time(NULL), hb);

	return 0;
}

