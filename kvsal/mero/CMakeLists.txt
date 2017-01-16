
SET(kvsal_LIB_SRCS
   kvsal_mero.c
)

add_library(kvsal SHARED ${kvsal_LIB_SRCS})
#target_link_libraries(kvsal hiredis)

