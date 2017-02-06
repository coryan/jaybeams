#!/bin/bash

set -e

./bootstrap
mkdir -p build/${VARIANT?} || :
cd build/${VARIANT?}
../../configure --prefix /home/${USER?}/jaybeams/staging/${VARIANT?}
make check
make install

exit 0
