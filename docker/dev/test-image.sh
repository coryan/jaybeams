#!/bin/sh

set -ev

mkdir build-jaybeams
cd build-jaybeams
git clone --depth=2 https://github.com/coryan/jaybeams.git
cd jaybeams
./bootstrap
./configure
make jb/ut_as_hhmmss
./jb/ut_as_hhmmss

exit 0