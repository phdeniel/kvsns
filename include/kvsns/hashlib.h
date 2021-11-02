/**
 * @file  hashlib.h
 * @brief Header file for sources of the HashLib API 
 */
#ifndef _HASHLIB_H
#define _HASHLIB_H

#define MAXSTRLEN 128

typedef uint32_t hashbuff128_t[4];

int hashlib_murmur3_128(char * grp, char * item, hashbuff128_t hb);
#endif /* _HASHLIB_H */

