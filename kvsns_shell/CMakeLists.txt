cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

add_executable(kvsns_busybox kvsns_busybox.c)
target_link_libraries(kvsns_busybox kvsns ${KVSAL_LIBRARY} ${STORE_LIBRARY})

add_executable(kvsns_cp kvsns_cp.c)
target_link_libraries(kvsns_cp kvsns)

add_custom_target(links DEPENDS kvsns_busybox)
add_custom_command(TARGET links
		   COMMAND ${CMAKE_COMMAND} -E remove ns_reset
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_reset
		   COMMAND ${CMAKE_COMMAND} -E remove ns_init
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_init
		   COMMAND ${CMAKE_COMMAND} -E remove ns_mkdir
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_mkdir
		   COMMAND ${CMAKE_COMMAND} -E remove ns_rmdir
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_rmdir
		   COMMAND ${CMAKE_COMMAND} -E remove ns_creat
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_creat
		   COMMAND ${CMAKE_COMMAND} -E remove ns_cd
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_cd
		   COMMAND ${CMAKE_COMMAND} -E remove ns_ln
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_ln
		   COMMAND ${CMAKE_COMMAND} -E remove ns_link
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_link
		   COMMAND ${CMAKE_COMMAND} -E remove ns_rm
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_rm
		   COMMAND ${CMAKE_COMMAND} -E remove ns_rename
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_rename
		   COMMAND ${CMAKE_COMMAND} -E remove ns_readlink
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_readlink
		   COMMAND ${CMAKE_COMMAND} -E remove ns_getattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_getattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_getxattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_getxattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_setattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_setattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_setxattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_setxattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_listxattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_listxattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_removexattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_removexattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_clearxattr
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_clearxattr
		   COMMAND ${CMAKE_COMMAND} -E remove ns_ls
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_ls
		   COMMAND ${CMAKE_COMMAND} -E remove ns_fsstat
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_fsstat
		   COMMAND ${CMAKE_COMMAND} -E remove ns_truncate
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_truncate
		   COMMAND ${CMAKE_COMMAND} -E remove ns_mr_proper
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_mr_proper
		   COMMAND ${CMAKE_COMMAND} -E remove ns_cp
		   COMMAND ${CMAKE_COMMAND} -E create_symlink kvsns_busybox ns_cp
		   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		   VERBATIM)

