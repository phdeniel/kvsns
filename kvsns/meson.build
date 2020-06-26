kvsns_lib_srcs = ['kvsns_init.c', 'kvsns_handle.c', 'kvsns_file.c', 'kvsns_internal.c', 'kvsns_xattr.c', 'kvsns_copy.c']
kvsns_lib = shared_library('kvsns',
			   kvsns_lib_srcs,
			   dependencies : [ini_config_lib, dl_lib])
kvsns_dep = declare_dependency(link_with: kvsns_lib)
