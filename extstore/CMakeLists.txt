if(USE_POSIX_STORE)
	add_subdirectory(posix_store)
endif(USE_POSIX_STORE)

if(USE_RADOS)
	add_subdirectory(rados)
endif(USE_RADOS)

if(USE_CRUD_CACHE)
	add_subdirectory(crud_cache)
endif(USE_CRUD_CACHE)
