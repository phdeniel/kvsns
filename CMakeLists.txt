cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

PROJECT(libkvsns C)
set(LIBKVSNS_MAJOR_VERSION 0)
set(LIBKVSNS_MINOR_VERSION 9)
set(LIBKVSNS_PATCH_LEVEL 3)
set(LIBKVSNS_EXTRA_VERSION -dev-2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

set(LIBKVSNS_BASE_VERSION ${LIBKVSNS_MAJOR_VERSION}.${LIBKVSNS_MINOR_VERSION}.${LIBKVSNS_PATCH_LEVEL})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "libkvsns version ${LIBKVSNS_BASE_VERSION}")

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

SET(kvsns_LIB_SRCS
    kvsal_redis.c
    kvsns_handle.c 
    kvsns_file.c 
    kvsns_internal.c
    kvsns_xattr.c 
)

add_library(kvsns SHARED ${kvsns_LIB_SRCS})
target_link_libraries(kvsns hiredis)

add_executable(kvsns_test kvsns_test.c)
target_link_libraries(kvsns_test kvsns hiredis)

add_subdirectory(kvsns_shell)

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

