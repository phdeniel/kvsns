
SET(extstore_crud_cache_cmd_LIB_SRCS
   extstore_crud_cache_cmd.c
)

add_library(extstore_crud_cache_cmd SHARED ${extstore_crud_cache_cmd_LIB_SRCS})
target_link_libraries(extstore_crud_cache_cmd hiredis ini_config)

install(TARGETS extstore_crud_cache_cmd DESTINATION lib)
