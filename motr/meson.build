motr_include =  include_directories('/usr/include/motr', '/usr/include/lustre')

m0common_lib_srcs = 'm0common.c'
m0common_lib = shared_library('m0common',
			      m0common_lib_srcs,
			      include_directories : motr_include,
			      dependencies : [ini_config_lib, 
					      pthread_lib,
					      uuid_lib,
					      yaml_lib,
					      motr_lib])

