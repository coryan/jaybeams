#!/bin/bash

# Exit on error
set -ev

# ... when testing in my workstation I need to set GEM_HOME ...
if [ "x${TRAVIS_BUILD_DIR}" != "x" -a "x${VARIANT}" == "xcov" ]; then
  # ... this is used to upload coverage information to coveralls.io
  gem install coveralls-lcov
fi

# ... make sure VERSION is set to something, even if it is an empty string ...
if [ "x${VERSION}" == "x" ]; then
    VERSION=""
fi

# ... only install the compiler that we are planning to use ...
if [ "x${COMPILER?}" == "xclang" ]; then
    sudo apt-get -qq -y install clang${VERSION?}
elif [ "x${COMPILER?}" == "xgcc" ]; then
    sudo apt-get -qq -y install g++${VERSION?}
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++${VERSION?} 90
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc${VERSION?} 90
    sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov${VERSION?} 90
else
    echo "Unknown compiler ${COMPILER?}"
    exit 1
fi

# ... install all the dependencies ...
sudo apt-get -qq -y install \
    autoconf-archive \
    automake \
    g++ \
    gcc \
    git \
    libboost1.55-all-dev \
    libfftw3-dev \
    libyaml-cpp-dev \
    make \
    wget

# ... we will only need doxygen if we are going to upload the documents ...
if [ "x${GENDOCS}" == "xyes" ]; then
    sudo apt-get -qq -y install doxygen
fi

# ... manually download Skye (one of my own libraries), extract it,
# compile it (it is a header-only library, but the tests are executed
# this way), and install it in /usr.  Notice how we set the CXX
# environment variable ...
wget -q https://github.com/coryan/Skye/releases/download/v0.3/skye-0.3.tar.gz
tar -xf skye-0.3.tar.gz
(source ${TRAVIS_BUILD_DIR?}/ci/before_script.sh && \
        cd skye-0.3 && \
        ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu && \
        make check && \
        sudo make install)

# ... manually download a recent version of lcov from a debian source
# repository, extract it, compile it and install it ...
sudo apt-get -qq -y install lcov

exit 0
