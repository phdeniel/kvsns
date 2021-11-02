ini_config = dependency('ini_config')
motr_incdir = include_directories('/usr/include/motr')

extstore_motr_lib_srcs = 'extstore_motr.c'
extstore_motr_lib = shared_library('extstore_motr',
				    extstore_motr_lib_srcs,
				    include_directories : motr_incdir,
				    link_with : [m0common_lib, hashkvs_lib],
				    dependencies : [ini_config_lib,
						    motr_lib,
						    dl_lib])

