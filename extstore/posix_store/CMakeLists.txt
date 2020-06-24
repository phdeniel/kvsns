
SET(extstore_posix_LIB_SRCS
   extstore_posix.c
)

add_library(extstore_posix SHARED ${extstore_posix_LIB_SRCS})

target_link_libraries(extstore_posix ini_config)

install(TARGETS extstore_posix DESTINATION lib)
