add_subdirectory(posix_store)

if (HAVE_LIBRADOS AND HAVE_RADOS_H)
	add_subdirectory(rados)
endif (HAVE_LIBRADOS AND HAVE_RADOS_H)

if (HAVE_MOTR AND HAVE_MOTR_H)
	add_subdirectory(motr)
endif (HAVE_MOTR AND HAVE_MOTR_H)

#add_subdirectory(crud_cache)
