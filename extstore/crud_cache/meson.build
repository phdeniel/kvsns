ini_config = dependency('ini_config')

extstore_crud_cache_lib_srcs = 'extstore_crud_cache.c'
extstore_crud_cache_lib = shared_library('extstore_crud_cache',
					  extstore_crud_cache_lib_srcs,
					  dependencies : ini_config_lib)


objstore_cmd_lib_srcs = 'objstore_cmd.c'
ini_config = dependency('ini_config')
objstore_cmd = shared_library('objstore_cmd',
			       objstore_cmd_lib_srcs,
			       dependencies : ini_config_lib)
