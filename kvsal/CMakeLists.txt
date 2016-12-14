if(USE_KVS_REDIS)
    add_subdirectory(redis)
endif(USE_KVS_REDIS)

if(USE_KVS_MERO)
    add_subdirectory(mero)
endif(USE_KVS_MERO)
