#!/bin/bash

rm -fr covdir
mkdir covdir

cov-build --dir covdir  cmake -DCMAKE_C_COMPILER=gcc  -DUSE_CRUD_CACHE=ON ..
cov-build --dir covdir  make
cov-import-scm --dir covdir --scm git
cov-analyze --dir covdir --all -s `pwd` -j auto --aggressiveness-level high
cov-commit-defects --host coverity.ccc.ocre.cea.fr --https-port 443 -ssl --scm git --dir covdir --stream kvsns --certs /etc/pki/ca-trust/source/anchors/ocre_ca.crt

