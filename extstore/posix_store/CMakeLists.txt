
SET(extstore_LIB_SRCS
   extstore.c
)

add_library(extstore SHARED ${extstore_LIB_SRCS})

target_link_libraries(extstore ini_config)

add_custom_command(TARGET extstore
                   COMMAND /usr/bin/cp libextstore.so ..)
