cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

add_executable(kvsns_script kvsns_script.c kvsns_do_op.c)
target_link_libraries(kvsns_script kvsns)


