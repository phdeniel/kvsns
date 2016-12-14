cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g -I..")

add_executable(kvsal_set_1 kvsal_set_1.c)
target_link_libraries(kvsal_set_1 kvsns hiredis ${STORE_LIBRARY})

add_executable(kvsal_get_1 kvsal_get_1.c)
target_link_libraries(kvsal_get_1 kvsns hiredis ${STORE_LIBRARY})

add_executable(kvsal_del_1 kvsal_del_1.c)
target_link_libraries(kvsal_del_1 kvsns hiredis ${STORE_LIBRARY})

add_executable(kvsal_exists_1 kvsal_exists_1.c)
target_link_libraries(kvsal_exists_1 kvsns hiredis ${STORE_LIBRARY})

add_executable(kvsal_set_many_transaction kvsal_set_many_transaction.c)
target_link_libraries(kvsal_set_many_transaction kvsns hiredis ${STORE_LIBRARY})

add_executable(kvsal_del_many_transaction kvsal_del_many_transaction.c)
target_link_libraries(kvsal_del_many_transaction kvsns hiredis ${STORE_LIBRARY})

add_executable(kvsal_get_list kvsal_get_list.c)
target_link_libraries(kvsal_get_list kvsns hiredis ${STORE_LIBRARY})
