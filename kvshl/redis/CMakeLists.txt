cmake_minimum_required(VERSION 2.6.3)

add_subdirectory(examples)


SET(kvshl_redis_LIB_SRCS
    kvshl_redis.c
)

add_library(kvshl_redis SHARED ${kvshl_redis_LIB_SRCS})

target_link_libraries(kvshl_redis
  hiredis
)

add_executable(kvshl_redis_test kvshl_redis_test.c)
target_link_libraries(kvshl_redis_test kvshl_redis hiredis)


