
SET(kvsal_LIB_SRCS
   kvsal_redis.c
)

add_library(kvsal_redis SHARED ${kvsal_LIB_SRCS})
target_link_libraries(kvsal_redis hiredis ini_config)

install(TARGETS kvsal_redis DESTINATION lib)
