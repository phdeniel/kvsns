cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

add_executable(kvsns_test kvsns_test.c)
target_link_libraries(kvsns_test kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY} )

add_executable(kvsns_file_test_unlink_on_close kvsns_file_test_unlink_on_close.c)
target_link_libraries(kvsns_file_test_unlink_on_close kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})

add_executable(kvsns_file_test_write kvsns_file_test_write.c)
target_link_libraries(kvsns_file_test_write kvsns ${STORE_LIBRARY}
		      ${KVSAL_LIBRARY})
