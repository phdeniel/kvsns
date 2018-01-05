
SET(kvsal_LIB_SRCS
   kvsal_redis.c
)

add_library(kvsal SHARED ${kvsal_LIB_SRCS})
target_link_libraries(kvsal hiredis ini_config)

add_custom_command(TARGET kvsal
                   COMMAND ${CMAKE_COMMAND} -E copy libkvsal.so ..)
