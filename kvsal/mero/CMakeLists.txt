set(LUSTRE_PATH "/usr/src/lustre-2.7.0")
set(MERO_PATH "/home/phil/devs_mero/mero")

set(MERO_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MERO_CFLAGS "${MERO_CFLAGS} -include config.h ")
set(MERO_CFLAGS "${MERO_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ") 
set(MERO_CFLAGS "${MERO_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ") 
set(MERO_CFLAGS "${MERO_CFLAGS} -I/usr/include/mero -I${MERO_PATH} -I${LUSTRE_PATH}/libcfs/include ")
set(MERO_CFLAGS "${MERO_CFLAGS} -I${LUSTRE_PATH}/lnet/include -I${LUSTRE_PATH}/lustre/include")

include_directories("/usr/include/mero")
#LIBS=-lm -lpthread -lrt -lgf_complete -lyaml -luuid -lmero 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MERO_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MERO_CFLAGS}")

SET(kvsal_LIB_SRCS
   m0kvs.c
   kvsal_mero.c
)

add_library(kvsal SHARED ${kvsal_LIB_SRCS})

target_link_libraries(kvsal
  mero
  uuid
  yaml
  gf_complete
)

