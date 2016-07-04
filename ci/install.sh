#!/bin/bash

# Exit on error
set -ev

# ... when testing in my workstation I need to set GEM_HOME ...
if [ "x${TRAVIS_BUILD_DIR}" != "x" -a "x${VARIANT}" == "xcov" ]; then
  # ... this is used to upload coverage information to coveralls.io
  gem install coveralls-lcov
fi

sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update

# ... install all the dependencies ...
sudo apt-get -qq -y install \
    automake \
    doxygen \
    g++-5 \
    gcc-5 \
    clang-3.6 \
    git \
    lcov \
    libboost1.55-all-dev \
    libfftw3-dev \
    libyaml-cpp-dev \
    make \
    tar \
    wget \
    xz-utils

sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-3.6 90
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.6 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 90
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 90
sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-5 90

# ... manually download and install a recent version of
# autoconf-archive, we need support for C++-14 detection ...
wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2016.03.20.tar.xz		
tar -xf autoconf-archive-2016.03.20.tar.xz		
(cd autoconf-archive-2016.03.20 && ./configure --prefix=/usr && \
        make && \
        sudo make install)

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

exit 0
