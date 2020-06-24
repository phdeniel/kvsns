add_subdirectory(posix_store)

if (HAVE_LIBRADOS AND HAVE_RADOS_H)
	add_subdirectory(rados)
endif (HAVE_LIBRADOS AND HAVE_RADOS_H)

add_subdirectory(crud_cache)
