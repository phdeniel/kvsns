Using the kvsns_script
======================

The kvsns_script is a simple program that makes it possible to use the KVSNS API from a "batch like" environment.
The program supports no variable nor loop controls or conditionnals. Later versions may do this, or will use a binding
to an high-level scripting language such as Golang or Lua.

Commands from kvsns_script are similar to aliases (symlinks) to kvsns_busybox, just remove the ns_ prefix.

kvsns_script verbs
-------------------

The following command can be use inside kvsns_script

* all line starting with *#* is a comment
* *reset* : set all navigation variable (CWD, current inode) to the default values
* *init* : performs the init of the namespace from an empty KVS
* *creat <newfile>* : creates a new file in the cirrent directory
* *ln <newdir> <content>*: creates a symlink in the current directory
* *readlink <link>*: reads the content of a symlink
* *mkdir <newdir>*: creates a new directoy in the current directory
* *rmdir <dirname>*: removes an (empty) directory
* *cd <dir>* : cd into a directory
* *ls* : lists the content of a directory
* *getattr <name>*: gets the attributes of a dirent
* *archive <name>*: archives a file in the current directory (crud_cache only)
* *restore <name>*: restores a file in the current directory (crud_cache only)
* *release <name>*: releases a file in the current directory (crud_cache only) 
* *state <name>*: displays the state of a file in the current directory (crud_cache only)
* *truncate <file> <offset>*: truncates a file
* *setxattr <name> <xattr_name> <xattr_val>*: sets an extended attrinute to a file
* *getxattr <name> <xattr_name>*: gets the value for an xattr set on a file
* *removexattr <name> <xattr_name>*: removes an xattr set on a file
* *listxattr <name>*: lists the xattr associated to a file
* *clearxattr <name>*: clears all xattr associated to a file
* *link <srcname> <newname>* or *link <srcname> <dstdir> <newname>*: creates a hardlink to an existing file
* *rm <file>*: removes (unlinks) a file
* *rename <sdir> <sname> <ddir> <dname>*: renames a file (changes its name and/or moves it to another directory)
* *fsstat*: returns FS statistics about the namespaces (call to statfs() )
* *mr_proper*: deletes **EVERY** key/value pairs inside the KVS, even those who do not belong to the KVSNS
* *sleep <seconds>*: sleeps (in seconds)
* *print <message>*: prints a message
* *timer_start*: starts the timer
* *timer_stop*: stops the timer and display the time since the latest call to timer_start
* *kvsal_set <key> <value>*: sets a key/value pair in the KVS
* *kvsal_get <key>*: gets a value by its key


