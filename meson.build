project('libkvsns',
	 'c',
	 version: '1.2.7',
	 license: 'LGPL3.0-or-later',
	 default_options : ['default_library=shared',
			     'werror=true'])

cc = meson.get_compiler('c')

hiredis_lib = cc.find_library('libhiredis', required: true)
rados_lib = cc.find_library('librados', required: false)
motr_lib = cc.find_library('libmotr', required: false)
dl_lib = cc.find_library('dl', required: true)
pthread_lib = cc.find_library('pthread', required: true)
ini_config_lib = cc.find_library('libini_config', required: true)

if motr_lib.found()
   uuid_lib = cc.find_library('libuuid', required: true)
   yaml_lib = cc.find_library('libyaml', required: true)
endif

options = cc.get_supported_arguments([
    '-Werror', 
    '-Wall'
])
add_project_arguments(options, language: 'c')

if motr_lib.found()
  motr_options = cc.get_supported_arguments([
    '-D_REENTRANT',
    '-D_GNU_SOURCE',
    '-DM0_INTERNAL= ',
    '-DM0_EXTERN=extern',
    '-Werror',
    '-Wall',
    '-Wno-attributes',
    '-Wno-unused-but-set-variable',
    '-fno-strict-aliasing',
    '-fno-omit-frame-pointer',
    '-fno-common',
    '-fPIC'])
  add_project_arguments(motr_options, language: 'c')
endif

if motr_lib.found()
   subdir('motr')
endif

subdir('hash')
subdir('extstore')
subdir('kvsal')
subdir('kvsns')
subdir('kvsns_shell')
subdir('kvsns_script')
subdir('kvsns_attach')
subdir('kvsns_hsm')
subdir('tests')

conf_data = configuration_data()
conf_data.set('LIBKVSNS_BASE_VERSION', meson.project_version())
conf_data.set('LIBKVSNS_EXTRA_VERSION', '')

distfile = meson.project_name() +  '-' + meson.project_version()
conf_data.set('CPACK_SOURCE_PACKAGE_FILE_NAME', distfile)

if rados_lib.found()
	conf_data.set('BCOND_RADOS','%bcond_without')
else
	conf_data.set('BCOND_RADOS', '%bcond_with')
endif

# Build the spec file
configure_file(input : 'libkvsns.spec-in.meson',
               output : 'libkvsns.spec',
               configuration : conf_data)

filespec = meson.source_root() + '/libkvsns.spec'
distdir = meson.build_root() + '/meson-dist/' + meson.project_name() +  '-' + meson.project_version()

meson.add_dist_script('/usr/bin/cp', filespec, distdir)
