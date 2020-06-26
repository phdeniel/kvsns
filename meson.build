# cmake_policy(['SET', 'CMP0017', 'NEW'])
project('libkvsns', 'c', default_options : ['default_library=static'])
libkvsns_major_version = '1'
libkvsns_minor_version = '2'
libkvsns_patch_level = '7'
libkvsns_extra_version = ''
cmake_c_flags = '${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g'
cmake_c_flags = '${CMAKE_C_FLAGS} -Wall -Werror'
libkvsns_base_version = [libkvsns_major_version, '.${LIBKVSNS_MINOR_VERSION}.${LIBKVSNS_PATCH_LEVEL}']
# message(['STATUS', 'cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}'])
# message(['STATUS', 'libkvsns version ${LIBKVSNS_BASE_VERSION}'])
# Option (for choosing KVSAL backend)
if 'USE_FSAL_LUSTRE'
  bcond_lustre = '%bcond_without'
else
  bcond_lustre = '%bcond_with'
endif
# BCOND* variables for spec file */
if 'USE_POSIX_STORE'
  bcond_posix_store = '%bcond_without'
else
  bcond_posix_store = '%bcond_with'
endif
# Final tuning
# Final tuning
## Check ini_config
# check_include_files(['ini_config.h', 'HAVE_INI_CONFIG_H'])
have_ini_config = find_library('HAVE_INI_CONFIG')
# check_library_exists(['ini_config', 'config_from_file', '', 'HAVE_INI_CONFIG'])
### Check for rados ###
# check_library_exists(['rados', 'rados_connect', '', 'HAVE_LIBRADOS'])
# check_include_files(['rados/librados.h', 'HAVE_RADOS_H'])
if (  NOT  HAVE_LIBRADOS ) OR (  NOT  HAVE_RADOS_H )
  # message(['STATUS', 'Cannot find librados'])
endif
if (  NOT  HAVE_INI_CONFIG ) OR (  NOT  HAVE_INI_CONFIG_H )
  # message(['FATAL_ERROR', 'Cannot find ini_config'])
endif
## Start building stuff ##
have_hiredis = find_library('HAVE_HIREDIS')
# check_library_exists(['hiredis', 'redisCommand', '', 'HAVE_HIREDIS'])
# check_include_files(['hiredis/hiredis.h', 'HAVE_HIREDIS_H'])
if (  NOT  HAVE_HIREDIS ) OR (  NOT  HAVE_HIREDIS_H )
  # message(['FATAL_ERROR', 'Cannot find hiredis'])
endif
if 'HAVE_RADOS_H', 'AND', 'HAVE_LIBRADOS'
  bcond_rados = '%bcond_without'
else
  bcond_rados = '%bcond_with'
endif
# Build ancillary libs
subdir('extstore')
subdir('kvsal')
subdir('kvsns')
subdir('tests')
subdir('kvsns_shell')
subdir('kvsns_attach')
# CPack / rpmbuild specific stuff
cpack_package_file_name = 'libkvsns-Source'
cpack_package_vendor = 'KVSNS'
cpack_package_description_summary = 'KVSNS - a namespace on top of a KVS'
cpack_package_version_major = libkvsns_major_version
cpack_package_version_minor = libkvsns_minor_version
cpack_package_version_patch = libkvsns_patch_level
# Tell CPack the kind of packages to be generated
cpack_generator = 'TGZ'
cpack_source_generator = 'TGZ'
cpack_source_ignore_files = '/.git/;/.gitignore/;/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}'
pkg_name = '${CPACK_PACKAGE_NAME}.tar.gz'
# add_custom_target(['dist', 'COMMAND', cmake_make_program, 'package_source'])
# Create the .pc file
# configure_file(['${PROJECT_SOURCE_DIR}/libkvsns.pc-in.cmake', '${PROJECT_SOURCE_DIR}/libkvsns.pc'])
# Now create a useable specfile
# configure_file(['${PROJECT_SOURCE_DIR}/libkvsns.spec-in.cmake', '${PROJECT_SOURCE_DIR}/libkvsns.spec'])
rpmdest = '--define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}''
# add_custom_target(['rpms', 'DEPENDS', 'dist'])
# add_custom_command(['TARGET', 'rpms', 'COMMAND', 'sh', '-c', 'rpmbuild ${RPMDEST} -ta ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz', 'VERBATIM', 'DEPENDS', 'dist'])
# add_custom_target(['rpm', 'DEPENDS', 'dist'])
# add_custom_command(['TARGET', 'rpm', 'COMMAND', 'sh', '-c', 'rpmbuild ${RPMDEST} -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz', 'VERBATIM', 'DEPENDS', 'dist'])
# add_custom_target(['srpm', 'DEPENDS', 'dist'])
# add_custom_command(['TARGET', 'srpm', 'COMMAND', 'sh', '-c', 'rpmbuild ${RPMDEST} -ts ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz', 'VERBATIM', 'DEPENDS', 'dist'])
# INSTALL
# install(['FILES', project_source_dir, '/libkvsns.pc', 'DESTINATION', 'lib'])
# install(['DIRECTORY', project_source_dir, '/include/kvsns', 'DESTINATION', 'include'])
#install(FILES ${PROJECT_SOURCE_DIR}/include/kvsns/kvsns.h DESTINATION include/kvsns)
#install(FILES ${PROJECT_SOURCE_DIR}/include/kvsns/kvsal.h DESTINATION include/kvsns)
##install(FILES ${PROJECT_SOURCE_DIR}/include/kvsns/extstore.h DESTINATION include/kvsns)
