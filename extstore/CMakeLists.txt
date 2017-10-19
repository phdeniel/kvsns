if(USE_POSIX_STORE)
	add_subdirectory(posix_store)
endif(USE_POSIX_STORE)

if(USE_POSIX_OBJ)
	add_subdirectory(posix_obj)
endif(USE_POSIX_OBJ)

if(USE_MERO_STORE)
	add_subdirectory(m0store)
endif(USE_MERO_STORE)

if(USE_RADOS)
	add_subdirectory(rados)
endif(USE_RADOS)

add_executable(extstore_cp_to extstore_cp_to.c)
target_link_libraries(extstore_cp_to extstore)

add_executable(extstore_cp_from extstore_cp_from.c)
target_link_libraries(extstore_cp_from extstore)

