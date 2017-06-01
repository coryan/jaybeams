#!/bin/bash

set -e

# Create the configure script and friends ...
./bootstrap

# ... build in a subdirectory, this is the common pattern in
# development anyway ...
mkdir build
cd build

# ... create Makefile and other files using the configuration
# parameters for this build ...
../configure ${CONFIGUREFLAGS} --prefix=$PWD/staging || cat config.log

# ... compile and run the tests ...
if [ "x${COVERITY}" = "xyes" -o "x${TRAVIS_PULL_REQUEST}" = "xfalse" ]; then
    # ... coverity builds are slow, so we disable them for pull
    # requests, where they cannot be uploaded anyway ...
    PATH=$PATH:/opt/coverity/cov-analysis-linux64-8.7.0/bin
    export PATH
    cov-build --dir cov-int make -j 2 check
    tar zcf jaybeams-coverity-upload.tgz cov-int
else
    make -j 2 check
fi

if [ "x${VALGRIND}" = "xyes" ]; then
    make -j 2 check-valgrind-memcheck
fi

# ... verify that the installation rules at least work ...
make install

# ... this script runs as root inside the docker image, make sure the
# user can write to the staging/ subdirectory ...
chmod 777 staging || true

exit 0

