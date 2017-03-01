
SET(extstore_LIB_SRCS
   extstore.c
)

add_library(extstore SHARED ${extstore_LIB_SRCS})

add_custom_command(TARGET extstore
                   COMMAND /usr/bin/cp libextstore.so ..)
