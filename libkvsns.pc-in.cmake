prefix=/usr
exec_prefix=/usr/bin
libdir=/usr/lib64
includedir=/usr/include

Name: @PROJECT_NAME@
Description: Library to access to a namespace inside a KVS
Requires:
Version: @LIBKVSNS_BASE_VERSION@
Cflags: -I/usr/include
Libs: -L/usr/lib64 -lkvsns -lhiredis

