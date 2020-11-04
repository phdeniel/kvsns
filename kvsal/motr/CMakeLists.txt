set(MOTR_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -include config.h ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/motr -I/usr/include/libcfs ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/motr")
#LIBS=-lm -lpthread -lrt -lyaml -luuid -lmotr 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MOTR_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MOTR_CFLAGS}")

SET(kvsal_LIB_SRCS
   kvsal_motr.c
)

add_library(kvsal_motr SHARED ${kvsal_LIB_SRCS})

target_link_libraries(kvsal_motr
  m0common
)

add_executable(m0kv_get m0kv_get.c)
target_link_libraries(m0kv_get kvsal_motr)

add_executable(m0kv_set m0kv_set.c)
target_link_libraries(m0kv_set kvsal_motr)

add_executable(m0kv_del m0kv_del.c)
target_link_libraries(m0kv_del kvsal_motr)
