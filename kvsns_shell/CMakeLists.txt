cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

add_executable(kvsns_busybox kvsns_busybox.c)
target_link_libraries(kvsns_busybox kvsns)

