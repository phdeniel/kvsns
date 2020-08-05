if(HAVE_HIREDIS AND HAVE_HIREDIS_H)
	add_subdirectory(redis)
endif(HAVE_HIREDIS AND HAVE_HIREDIS_H)

if (HAVE_MERO AND HAVE_MERO_H)
	add_subdirectory(mero)
endif (HAVE_MERO AND HAVE_MERO_H)
