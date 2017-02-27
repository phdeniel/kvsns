if(USE_POSIX_STORE)
	add_subdirectory(posix_store)
endif(USE_POSIX_STORE)

if(USE_POSIX_OBJ)
	add_subdirectory(posix_obj)
endif(USE_POSIX_OBJ)

if(USE_MERO_STORE)
	add_subdirectory(m0store)
endif(USE_MERO_STORE)

