add_subdirectory(posix_store)

if (HAVE_LIBRADOS AND HAVE_RADOS_H)
	add_subdirectory(rados)
endif (HAVE_LIBRADOS AND HAVE_RADOS_H)

if (HAVE_MERO AND HAVE_MERO_H)
	add_subdirectory(mero)
endif (HAVE_MERO AND HAVE_MERO_H)

add_subdirectory(crud_cache)
