set(MERO_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MERO_CFLAGS "${MERO_CFLAGS} -include config.h ")
set(MERO_CFLAGS "${MERO_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MERO_CFLAGS "${MERO_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(MERO_CFLAGS "${MERO_CFLAGS} -I/usr/include/mero -I/usr/include/libcfs ")
set(MERO_CFLAGS "${MERO_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/mero")

#LIBS=-lm -lpthread -lrt -lgf_complete -lyaml -luuid -lmero 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MERO_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MERO_CFLAGS}")

SET(extstore_LIB_SRCS
   extstore.c
)

add_library(extstore_mero SHARED ${extstore_LIB_SRCS})

target_link_libraries(extstore_mero
  m0common
  hiredis
  ini_config
  mero
  uuid
  yaml
  gf_complete
  ${SYSTEM_LIBRARIES} 
)

add_executable(m0store_cp_to m0store_cp_to.c)
target_link_libraries(m0store_cp_to extstore_mero)

add_executable(m0store_cp_from m0store_cp_from.c)
target_link_libraries(m0store_cp_from extstore_mero)


