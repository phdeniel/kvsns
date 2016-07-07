cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

add_executable(kvsns_busybox kvsns_busybox.c)
target_link_libraries(kvsns_busybox kvsns)

add_custom_target(links DEPENDS kvsns_busybox)
add_custom_command(TARGET links
                  COMMAND ln -s kvsns_busybox ns_reset
		  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  VERBATIM
		  DEPENDS kvsns_busybox)


