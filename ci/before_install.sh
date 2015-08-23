#!/bin/sh

# Exit on error
set -e

# ... first install a utility to quickly modify the apt sources ...
sudo apt-get -qq -y install python-software-properties

# ... this PPA contains backports of gcc and the related tools ...
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

# ... this PPA contains backports of many GNU tools, including
# automake and autoconf ...
sudo add-apt-repository -y ppa:dns/gnu

# ... this PPA contains backports of the Boost libraries, as of this
# writing they are on 1.55 while Boost just released 1.59.  The 1.55
# version is enough for me though ...
sudo add-apt-repository -y ppa:boost-latest/ppa

# ... these are the common toolchain utilities for clang and llvm ...
sudo add-apt-repository -y "deb http://llvm.org/apt/precise/ llvm-toolchain-precise main"

# ... here is the version of clang ...
sudo add-apt-repository -y "deb http://llvm.org/apt/precise/ llvm-toolchain-precise-${VERSION?} main"

# ... we need to install the public GPG keys from the llvm.org folks ...
wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -

# ... update the apt information after all the modifications are done ...
sudo apt-get -qq update

# ... this is used to upload coverage information to coveralls.io
gem install coveralls-lcov

# ... only install the compiler that we are planning to use ...
if [ "x${COMPILER?}" == "xclang" ]; then
    sudo apt-get -qq -y install clang${VERSION}
elif [ "x${COMPILER?}" == "xclang" ]; then
    sudo apt-get -qq -y install g++${VERSION}
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${VERSION?} 90
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${VERSION?} 90
    sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-${VERSION?} 90
else
    echo "Unknown compiler ${COMPILER?}"
    exit 1
fi

# ... install all the boost libraries ...
sudo apt-get -qq -y install boost1.55

# ... install a recent version of autoconf, automake and whatever make
# comes through that long list of apt sources.  We do not install
# autoconf because we will need to manually install it ...
sudo apt-get -qq -y install autoconf automake make

# ... we will only need doxygen if we are going to upload the documents ...
if [ "x${GENDOCS}" == "xyes" ]; then
    sudo apt-get -qq -y install doxygen
fi

# ... manually download the a sufficiently recent autoconf-archive
# extract it, compile it and install it in the /usr prefix ...
wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2015.02.24.tar.xz
tar -xf autoconf-archive-2015.02.24.tar.xz
(cd autoconf-archive-2015.02.24 && ./configure --prefix=/usr && make && sudo make install)

# ... we are going to need cmain to compile yaml-cpp ...
sudo apt-get -qq -y install cmake

# ... manually download the version of yaml-cpp that works with
# JayBeams, extract it, compile it, and install it in /usr ...
wget -q https://github.com/jbeder/yaml-cpp/archive/release-0.5.1.tar.gz
tar -xf release-0.5.1.tar.gz
(cd yaml-cpp-release-0.5.1 && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr .. && make && make test && sudo make install)

# ... manually download Skye (one of my own libraries), extract it,
# compile it (it is a header-only library, but the tests are executed
# this way), and install it in /usr.  Notice how we set the CXX
# environment variable ...
wget -q https://github.com/coryan/Skye/releases/download/v0.2/skye-0.2.tar.gz
tar -xf skye-0.2.tar.gz
(source ci/before_script.sh && cd skye-0.2 && ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu/ && make check && sudo make install)

# ... manually download a recent version of lcov from a debian source
# repository, extract it, compile it and install it ...
wget -q http://http.debian.net/debian/pool/main/l/lcov/lcov_1.11.orig.tar.gz
tar xf lcov_1.11.orig.tar.gz
sudo make -C lcov-1.11/ install

exit 0
