cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

PROJECT(libkvsns C)
set(LIBKVSNS_MAJOR_VERSION 1)
set(LIBKVSNS_MINOR_VERSION 1)
set(LIBKVSNS_PATCH_LEVEL 1)
set(LIBKVSNS_EXTRA_VERSION -rc-1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

set(LIBKVSNS_BASE_VERSION ${LIBKVSNS_MAJOR_VERSION}.${LIBKVSNS_MINOR_VERSION}.${LIBKVSNS_PATCH_LEVEL})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "libkvsns version ${LIBKVSNS_BASE_VERSION}")

# Option (for choosing KVSAL backend)
option(USE_KVS_REDIS "Use REDIS as a KVS in KVSAL" ON)
option(USE_KVS_MERO "USE MERO as a KVS in KVSAL" OFF)

option(USE_POSIX_STORE "Use POSIX directory as object store" ON)
option(USE_MERO_STORE "USE MERO as object store" OFF)
option(USE_POSIX_OBJ "USE POSIX with objs and keys" OFF)

if(USE_KVS_MERO)
  set(USE_KVS_REDIS OFF)
  message(STATUS "Disabling REDIS")
endif(USE_KVS_MERO)

if (USE_POSIX_OBJ OR USE_MERO_STORE)
  set(USE_POSIX_STORE OFF)
  message(STATUS "Disabling POSIX Store")
endif(USE_POSIX_OBJ OR USE_MERO_STORE)

message(STATUS "USE_KVS_REDIS=${USE_KVS_REDIS}")
message(STATUS "USE_KVS_MERO=${USE_KVS_MERO}")
message(STATUS "USE_POSIX_STORE=${USE_POSIX_STORE}")
message(STATUS "USE_MERO_STORE=${USE_MERO_STORE}")
message(STATUS "USE_POSIX_OBJ=${USE_POSIX_OBJ}")

## Start building stuff ##
if(USE_KVS_REDIS)
include(CheckIncludeFiles)
include(CheckLibraryExists)

find_library(HAVE_HIREDIS hiredis)
  check_library_exists(
    hiredis
    redisCommand
    ""
    HAVE_HIREDIS
    )
check_include_files("hiredis/hiredis.h" HAVE_HIREDIS_H)

if((NOT HAVE_HIREDIS) OR (NOT HAVE_HIREDIS_H))
      message(FATAL_ERROR "Cannot find hiredis")
endif((NOT HAVE_HIREDIS) OR (NOT HAVE_HIREDIS_H))
endif(USE_KVS_REDIS)

# Build ancillary libs
add_subdirectory(extstore)
set(STORE_LIBRARY extstore)

add_subdirectory(kvsal)
set(KVSAL_LIBRARY kvsal)

# Now compile kvsns #
SET(kvsns_LIB_SRCS
    kvsns_handle.c
    kvsns_file.c
    kvsns_internal.c
    kvsns_xattr.c
)

add_library(kvsns SHARED ${kvsns_LIB_SRCS})
target_link_libraries(kvsns ${STORE_LIBRARY} ${KVSAL_LIBRARY})

add_subdirectory(tests)
add_subdirectory(kvsns_shell)
add_subdirectory(kvsal_non_reg)

# CPack / rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "libkvsns-Source" )
set(CPACK_PACKAGE_VENDOR "KVSNS")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KVSNS - a namespace on top of a KVS")
SET(CPACK_PACKAGE_VERSION_MAJOR ${LIBKVSNS_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${LIBKVSNS_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${LIBKVSNS_PATCH_LEVEL})

# Tell CPack the kind of packages to be generated
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

set(CPACK_SOURCE_IGNORE_FILES
  "/.git/;/.gitignore/;/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")

include(CPack)

set(PKG_NAME "${CPACK_PACKAGE_NAME}.tar.gz")
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# Create the .pc file
configure_file(
  "${PROJECT_SOURCE_DIR}/libkvsns.pc-in.cmake"
  "${PROJECT_SOURCE_DIR}/libkvsns.pc"
)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/libkvsns.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/libkvsns.spec"
)

set(RPMDEST "--define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}'")

add_custom_target( rpms DEPENDS dist)
add_custom_command(TARGET rpms
                  COMMAND sh -c "rpmbuild ${RPMDEST} -ta ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

add_custom_target( rpm DEPENDS dist)
add_custom_command(TARGET rpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

add_custom_target( srpm DEPENDS dist)
add_custom_command(TARGET srpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} -ts ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

