cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

add_executable(kvsns_attach kvsns_attach.c)
target_link_libraries(kvsns_attach kvsns ${KVSAL_LIBRARY} ${STORE_LIBRARY})

