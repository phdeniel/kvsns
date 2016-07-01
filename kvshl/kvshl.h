#ifndef _KVSHL_H
#define  _KVSHL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define KLEN 256
#define VLEN 256
int kvshl_init();
int kvshl_set_char(char *k, char *v);
int kvshl_get_char(char *k, char *v);
int kvshl_del(char *k);
int kvshl_incr_counter(char *k, unsigned long long *v);
#endif

