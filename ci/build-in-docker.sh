#!/bin/sh

set -e

# Create the configure script and friends ...
./bootstrap

# ... build in a subdirectory, this is the common pattern in
# development anyway ...
mkdir build
cd build

# ... create Makefile and other files using the configuration
# parameters for this build ...
../configure ${CONFIGUREFLAGS} --prefix=$PWD/staging

# ... compile and run the tests ...
make -j 2 check

if [ "x${VALGRIND}" = "xyes" ]; then
    make -j 2 check-valgrind-memcheck
fi

# ... verify that the installation rules at least work ...
make install

# ... this script runs as root inside the docker image, make sure the
# user can write to the staging/ subdirectory ...
chmod 777 staging || true

exit 0

