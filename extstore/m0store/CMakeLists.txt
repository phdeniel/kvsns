set(LUSTRE_PATH "/usr/src/lustre-2.7.0")
set(MERO_PATH "/root/mero_master")

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

SET(extstore_LIB_SRCS
   extstore.c
   m0store.c
)

add_library(extstore SHARED ${extstore_LIB_SRCS})

target_link_libraries(extstore
  mero
  uuid
  yaml
  gf_complete
  ${SYSTEM_LIBRARIES} 
)

add_executable(m0store_cp_to m0store_cp_to.c)
target_link_libraries(m0store_cp_to extstore)

add_executable(m0store_cp_from m0store_cp_from.c)
target_link_libraries(m0store_cp_from extstore)

add_custom_command(TARGET extstore
                   COMMAND /usr/bin/cp libextstore.so ..)

