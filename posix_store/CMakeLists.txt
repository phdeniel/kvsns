
SET(posix_store_LIB_SRCS
   extstore.c
)

#add_library(posix_store STATIC ${posix_store_LIB_SRCS})
add_library(posix_store SHARED ${posix_store_LIB_SRCS})

