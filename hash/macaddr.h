/**
 * @file  hashlib.h
 * @brief Header file for sources of the HashLib API 
 */
#ifndef _MACADDR_H
#define _MACADDR_H

typedef unsigned char mac_addr_t[6];

int get_mac_addr(mac_addr_t mac_address);

#endif /* _MACADDR_H */

