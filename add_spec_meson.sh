#!/bin/sh

set -o xtrace

tarfile=$1
source_root=$2

mkdir /tmp/$$/
cd  /tmp/$$/
tar xf $tarfile
cd libkvsns*
cp $source_root/libkvsns.spec .
cd ..
rm $tarfile
libkvsns=`ls -d libkvsns*`
tar cJf $tarfile $libkvsns/*
rm -fr /tmp/$$/
sha256sum $tarfile > $tarfile.sha256sum

