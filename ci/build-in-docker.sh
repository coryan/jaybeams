#!/bin/bash

set -e

# Create the configure script and friends ...
./bootstrap

# ... build in a subdirectory, this is the common pattern in
# development anyway ...
mkdir build
cd build

# ... the gRPC++ and Protobuf libraries are installed in /usr/local
# and the pkg-config files for them are in /usr/local/lib/pkgconfig,
# which is not the usual search path
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
export PKG_CONFIG_PATH

# ... create Makefile and other files using the configuration
# parameters for this build ...
../configure ${CONFIGUREFLAGS} --prefix=$PWD/staging || cat config.log

# ... compile and run the tests ...
do_coverity=no
if [ "x${TRAVIS_PULL_REQUEST}" = "xtrue" ]; then
    # Always skip Coverity Scan builds for PR ...
    echo "This is a pull request, skipping coverity build"
elif [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
    # ... and for branches other than master ...
    echo "Building on the ${TRAVIS_BRANCH} branch, skipping coverity build"
elif [ "x${TRAVIS_EVENT_TYPE}" = "xcron" -a "x${COVERITY}" = "xyes" ]; then
    # ... and really, only do them weekly ...
    echo "Enabling coverity build"
    do_coverity=yes
fi

echo "Travis event type: " ${TRAVIS_EVENT_TYPE}
echo "Coverity: " ${COVERITY}
echo "do coverity: " ${do_coverity}

if [ "x${do_coverity}" = "xyes" ]; then
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

