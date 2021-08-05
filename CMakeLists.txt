cmake_minimum_required(VERSION 2.6.4)
cmake_policy(SET CMP0017 NEW)

PROJECT(libkvsns C)
set(LIBKVSNS_MAJOR_VERSION 1)
set(LIBKVSNS_MINOR_VERSION 2)
set(LIBKVSNS_PATCH_LEVEL 11)
set(LIBKVSNS_EXTRA_VERSION "")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(LIBKVSNS_BASE_VERSION ${LIBKVSNS_MAJOR_VERSION}.${LIBKVSNS_MINOR_VERSION}.${LIBKVSNS_PATCH_LEVEL})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "libkvsns version ${LIBKVSNS_BASE_VERSION}")

# Final tuning
# Final tuning

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

## Check for MOTR
find_library(HAVE_MOTR motr)
check_library_exists(
        motr
        m0_init
        ""
        HAVE_MOTR
	)
check_include_files("motr/config.h" HAVE_MOTR_H)

### Check for rados ###
check_library_exists(
	rados
	rados_connect
	""
	HAVE_LIBRADOS
	)
check_include_files("rados/librados.h" HAVE_RADOS_H)

if((NOT HAVE_LIBRADOS) OR (NOT HAVE_RADOS_H))
      message(STATUS "Cannot find librados")
endif((NOT HAVE_LIBRADOS) OR (NOT HAVE_RADOS_H))


## Check for HIREDIS ##

find_library(HAVE_HIREDIS hiredis)
check_library_exists(
    hiredis
    redisCommand
    ""
    HAVE_HIREDIS
    )
check_include_files("hiredis/hiredis.h" HAVE_HIREDIS_H)

## Set BCOND for spec file management */
if (HAVE_RADOS_H AND HAVE_LIBRADOS)
	set(BCOND_RADOS "%bcond_without")
else (HAVE_RADOS_H AND HAVE_LIBRADOS)
	set(BCOND_RADOS "%bcond_with")
endif (HAVE_RADOS_H AND HAVE_LIBRADOS)

if (HAVE_HIREDIS_H AND HAVE_HIREDIS)
	set(BCOND_REDIS "%bcond_without")
else (HAVE_HIREDIS_H AND HAVE_HIREDIS)
	set(BCOND_REDIS "%bcond_with")
endif (HAVE_HIREDIS_H AND HAVE_HIREDIS)


if (HAVE_MOTR_H AND HAVE_MOTR)
	set(BCOND_MOTR "%bcond_without")
else (HAVE_MOTR_H AND HAVE_MOTR)
	set(BCOND_MOTR "%bcond_with")
endif (HAVE_MOTR_H AND HAVE_MOTR)

# Check if a KVS if available
set(HAVE_KVS FALSE)

if (HAVE_HIREDIS_H AND HAVE_HIREDIS)
	set(HAVE_KVS TRUE)
endif (HAVE_HIREDIS_H AND HAVE_HIREDIS)

if (HAVE_MOTR_H AND HAVE_MOTR)
	set(HAVE_KVS TRUE)
endif (HAVE_MOTR_H AND HAVE_MOTR)

if (NOT HAVE_KVS)
	message(FATAL_ERROR "Can't find a KVS")
endif (NOT HAVE_KVS)

# Build ancillary libs
add_subdirectory(extstore)
add_subdirectory(kvsal)
add_subdirectory(kvsns)
add_subdirectory(tests)
add_subdirectory(kvsns_shell)
add_subdirectory(kvsns_script)
add_subdirectory(kvsns_attach)
add_subdirectory(kvsns_hsm)
if (HAVE_MOTR AND HAVE_MOTR_H)
	add_subdirectory(motr)
endif (HAVE_MOTR AND HAVE_MOTR_H)


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

# INSTALL
install(FILES ${PROJECT_SOURCE_DIR}/libkvsns.pc DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/kvsns DESTINATION include)
