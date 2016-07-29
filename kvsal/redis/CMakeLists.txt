cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

add_subdirectory(examples)


SET(kvsal_redis_LIB_SRCS
    kvsal_redis.c
)

add_library(kvsal_redis STATIC  ${kvsal_redis_LIB_SRCS})

target_link_libraries(kvsal_redis
  hiredis
)

add_executable(kvsal_redis_test kvsal_redis_test.c)
target_link_libraries(kvsal_redis_test kvsal_redis hiredis)


