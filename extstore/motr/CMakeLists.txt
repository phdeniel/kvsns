set(MOTR_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -include config.h ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/motr -I/usr/include/libcfs ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/motr")

#LIBS=-lm -lpthread -lrt  -lyaml -luuid -lmotr 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MOTR_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MOTR_CFLAGS}")

SET(extstore_LIB_SRCS
   extstore_motr.c
)

add_library(extstore_motr SHARED ${extstore_LIB_SRCS})

target_link_libraries(extstore_motr
  m0common
  hashkvs
  ini_config
  motr
  uuid
  yaml
  ${SYSTEM_LIBRARIES} 
)

add_executable(m0store_cp_to m0store_cp_to.c)
target_link_libraries(m0store_cp_to extstore_motr)

add_executable(m0store_cp_to_bulk m0store_cp_to_bulk.c)
target_link_libraries(m0store_cp_to_bulk extstore_motr)

add_executable(m0store_cp_from m0store_cp_from.c)
target_link_libraries(m0store_cp_from extstore_motr)

add_executable(m0store_cp_from_bulk m0store_cp_from_bulk.c)
target_link_libraries(m0store_cp_from_bulk extstore_motr)

