project('libkvsns', 'c', default_options : ['default_library=shared'])
libkvsns_major_version = '1'
libkvsns_minor_version = '2'
libkvsns_patch_level = '7'
libkvsns_extra_version = ''
cmake_c_flags = '${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g'
cmake_c_flags = '${CMAKE_C_FLAGS} -Wall -Werror'
libkvsns_base_version = [libkvsns_major_version, '.${LIBKVSNS_MINOR_VERSION}.${LIBKVSNS_PATCH_LEVEL}']

cc = meson.get_compiler('c')

ini_config_lib = cc.find_library('libini_config', required: true)
hiredis_lib = cc.find_library('libhiredis', required: true)
rados_lib = cc.find_library('librados', required: false)
dl_lib = cc.find_library('dl', required: true)

subdir('extstore')
subdir('kvsal')
subdir('kvsns')
subdir('kvsns_shell')
subdir('kvsns_attach')
subdir('tests')
