
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

SET(m0common_LIB_SRCS
   m0common.c
)

add_library(m0common SHARED ${m0common_LIB_SRCS})

target_link_libraries(m0common
  mero
  uuid
  yaml
  gf_complete
  ini_config
)
