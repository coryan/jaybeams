#!/bin/bash

set -e

NCPU=$(grep -c ^processor /proc/cpuinfo 2>/dev/null)
if [ -z "${NCPU}" ]; then
    NCPU=1
fi

./bootstrap
mkdir -p build/${VARIANT?} || :
cd build/${VARIANT?}
CXXFLAGS=-O3 ../../configure --prefix /home/${USER?}/jaybeams/staging/${VARIANT?}
make check -j ${NCPU?}
make install

exit 0
