cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

PROJECT(libkvsns C)
set(LIBKVSNS_MAJOR_VERSION 1)
set(LIBKVSNS_MINOR_VERSION 2)
set(LIBKVSNS_PATCH_LEVEL 3)
set(LIBKVSNS_EXTRA_VERSION "")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(LIBKVSNS_BASE_VERSION ${LIBKVSNS_MAJOR_VERSION}.${LIBKVSNS_MINOR_VERSION}.${LIBKVSNS_PATCH_LEVEL})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "libkvsns version ${LIBKVSNS_BASE_VERSION}")

# Option (for choosing KVSAL backend)
option(USE_KVS_REDIS "Use REDIS as a KVS in KVSAL" ON)
option(USE_KVS_MERO "Use MERO as a KVS in KVSAL" OFF)

option(USE_POSIX_STORE "Use POSIX directory as object store" ON)
option(USE_POSIX_OBJ "Use POSIX with objs and keys" OFF)
option(USE_RADOS "Use Ceph/RADOS via librados" OFF)

if(USE_FSAL_LUSTRE)
    set(BCOND_LUSTRE "%bcond_without")
else(USE_FSAL_LUSTRE)
    set(BCOND_LUSTRE "%bcond_with")
endif(USE_FSAL_LUSTRE)

# BCOND* variables for spec file */
if (USE_KVS_REDIS)
	set(BCOND_KVS_REDIS "%bcond_without")
else (USE_KVS_REDIS)
	set(BCOND_KVS_REDIS "%bcond_with")
endif (USE_KVS_REDIS)

if (USE_POSIX_STORE)
	set(BCOND_POSIX_STORE "%bcond_without")
else (USE_POSIX_STORE)
	set(BCOND_POSIX_STORE "%bcond_with")
endif (USE_POSIX_STORE)

if (USE_POSIX_OBJ)
	set(BCOND_POSIX_OBJ "%bcond_without")
else (USE_POSIX_OBJ)
	set(BCOND_POSIX_OBJ "%bcond_with")
endif (USE_POSIX_OBJ)

if (USE_RADOS)
	set(BCOND_RADOS "%bcond_without")
else (USE_RADOS)
	set(BCOND_RADOS "%bcond_with")
endif (USE_RADOS)

# Final tuning
if (USE_POSIX_OBJ OR USE_RADOS)
  set(USE_POSIX_STORE OFF)
  message(STATUS "Disabling POSIX Store")
endif(USE_POSIX_OBJ OR USE_RADOS)

message(STATUS "USE_KVS_REDIS=${USE_KVS_REDIS}")
message(STATUS "USE_POSIX_STORE=${USE_POSIX_STORE}")
message(STATUS "USE_POSIX_OBJ=${USE_POSIX_OBJ}")
message(STATUS "USE_RADOS=${USE_RADOS}")


include(CheckIncludeFiles)
include(CheckLibraryExists)

## Check ini_config
check_include_files("ini_config.h" HAVE_INI_CONFIG_H)
find_library(HAVE_INI_CONFIG ini_config)
check_library_exists(
	ini_config
	config_from_file
	""
	HAVE_INI_CONFIG
	)


if((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))
      message(FATAL_ERROR "Cannot find ini_config")
endif((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))

## Start building stuff ##
if(USE_KVS_REDIS)


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

### Check for rados ###
if(USE_RADOS)
check_library_exists(
	rados
	rados_connect
	""
	HAVE_LIBRADOS
	)
check_include_files("rados/librados.h" HAVE_RADOS_H)

if((NOT HAVE_LIBRADOS) OR (NOT HAVE_RADOS_H))
      message(FATAL_ERROR "Cannot find librados")
endif((NOT HAVE_LIBRADOS) OR (NOT HAVE_RADOS_H))

endif(USE_RADOS)


# Build ancillary libs
add_subdirectory(extstore)
set(STORE_LIBRARY extstore)

add_subdirectory(kvsal)
set(KVSAL_LIBRARY kvsal)

add_subdirectory(kvsns)

add_subdirectory(tests)
add_subdirectory(kvsns_shell)
add_subdirectory(kvsns_attach)
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

