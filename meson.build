project('libkvsns',
	 'c',
	 version: '1.2.7',
	 license: 'LGPL3.0-or-later',
	 default_options : ['default_library=shared',
			     'werror=true'])
cmake_c_flags = '${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g'
cmake_c_flags = '${CMAKE_C_FLAGS} -Wall -Werror'

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
