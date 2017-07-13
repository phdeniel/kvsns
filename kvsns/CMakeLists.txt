SET(kvsns_LIB_SRCS
    kvsns_init.c
    kvsns_handle.c
    kvsns_file.c
    kvsns_internal.c
    kvsns_xattr.c
    kvsns_copy.c
)

add_library(kvsns SHARED ${kvsns_LIB_SRCS})
target_link_libraries(kvsns ini_config ${STORE_LIBRARY} ${KVSAL_LIBRARY})

