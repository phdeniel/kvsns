
SET(hashkvs_LIB_SRCS
   hashlib.c 
   murmur3.c 
   macaddr.c
)

add_library(hashkvs SHARED ${hashkvs_LIB_SRCS})

install(TARGETS hashkvs DESTINATION lib)
