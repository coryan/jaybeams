#!/bin/bash

set -e

./bootstrap
mkdir -p build/${VARIANT?} || :
cd build/${VARIANT?}
CXXFLAGS=-O3 ../../configure --prefix /home/${USER?}/jaybeams/staging/${VARIANT?}
make check
make install

exit 0
