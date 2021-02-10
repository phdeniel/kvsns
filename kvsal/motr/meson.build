kvsal_motr_lib_srcs = 'kvsal_motr.c'
motr_incdir = include_directories('/usr/include/motr')
kvsal_motr_lib = shared_library('kvsal_motr',
				 kvsal_motr_lib_srcs,
				 include_directories : motr_incdir,
				 link_with : m0common_lib,
				 dependencies : [ini_config_lib, motr_lib])
