
SET(posix_obj_LIB_SRCS
   extstore.c
)

add_library(posix_obj SHARED ${posix_obj_LIB_SRCS})
target_link_libraries(posix_obj hiredis)


