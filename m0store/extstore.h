/* -*- C -*- */
#ifndef EXTSTORE_H
#define EXTSTORE_H 

#include <libzfswrap.h>

int init_clovis(char *clovis_local_addr,
	        char *clovis_ha_addr,
		char *clovis_confd_addr,
		char *clovis_prof,
		char *clovis_index_dir);

void fini_clovis(void);

void thread_enroll();
void thread_quit();

int create_object(inogen_t *ino);
int delete_object(inogen_t *ino);

int pread_clovis(inogen_t *ino,
                 off_t   offset,
                 size_t length,
                 char *data);

int pwrite_clovis(inogen_t *ino,
                  off_t   offset,
                  size_t length,
                  char *data);

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
#endif
