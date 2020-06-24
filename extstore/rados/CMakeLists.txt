
SET(extstore_rados_LIB_SRCS
   extstore_rados.c
)

add_library(extstore_rados SHARED ${extstore_rados_LIB_SRCS})
target_link_libraries(extstore_rados rados ini_config)

install(TARGETS extstore_rados DESTINATION lib)
