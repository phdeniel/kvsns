add_subdirectory(examples)

add_executable(kvshl_redis_test kvshl_redis_test.c kvshl_redis.c)
target_link_libraries(kvshl_redis_test hiredis)

