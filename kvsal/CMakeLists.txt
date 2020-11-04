if(HAVE_HIREDIS AND HAVE_HIREDIS_H)
	add_subdirectory(redis)
endif(HAVE_HIREDIS AND HAVE_HIREDIS_H)

if (HAVE_MOTR AND HAVE_MOTR_H)
	add_subdirectory(motr)
endif (HAVE_MOTR AND HAVE_MOTR_H)
