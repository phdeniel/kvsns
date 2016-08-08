
SET(posix_store_LIB_SRCS
   extstore.c
   extstore.h
)

add_library(posix_store STATIC ${posix_store_LIB_SRCS})

