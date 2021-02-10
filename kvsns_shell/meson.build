kvsns_busybox_exe = executable('kvsns_busybox',
			       'kvsns_busybox.c',
			        dependencies: kvsns_dep)

kvsns_cp_exe = executable('kvsns_cp',
			  'kvsns_cp.c',
			   dependencies: kvsns_dep)

busybox_path=meson.build_root() + '/kvsns_shell/kvsns_busybox'

foreach t: ['ns_archive', 'ns_cd', 'ns_clearxattr', 'ns_cp', 'ns_creat',
	    'ns_fsstat', 'ns_getattr', 'ns_getxattr', 'ns_init', 'ns_link',
	    'ns_listxattr', 'ns_ln', 'ns_ls', 'ns_mkdir', 'ns_mr_proper',
	    'ns_readlink', 'ns_release', 'ns_removexattr', 'ns_rename',
	    'ns_reset', 'ns_restore', 'ns_rm', 'ns_rmdir', 'ns_setxattr',
	    'ns_state', 'ns_truncate']
	run_command('/usr/bin/ln', '-sf', busybox_path,
		    meson.build_root() + '/kvsns_shell/' + t)
endforeach

