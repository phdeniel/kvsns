
SET(kvsal_LIB_SRCS
   kvsal_redis.c
)

add_library(kvsal SHARED ${kvsal_LIB_SRCS})
target_link_libraries(kvsal hiredis)

add_custom_command(TARGET kvsal
                   COMMAND /usr/bin/cp libkvsal.so ..)
