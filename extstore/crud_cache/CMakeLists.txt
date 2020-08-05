
SET(extstore_crud_cache_LIB_SRCS
    extstore_crud_cache.c
)

add_library(extstore_crud_cache SHARED ${extstore_crud_cache_LIB_SRCS})
target_link_libraries(extstore_crud_cache ini_config dl)

install(TARGETS extstore_crud_cache DESTINATION lib)


SET(objstore_cmd_LIB_SRCS
    objstore_cmd.c
)

add_library(objstore_cmd SHARED ${objstore_cmd_LIB_SRCS})
target_link_libraries(objstore_cmd ini_config)

install(TARGETS objstore_cmd DESTINATION lib)
