#!/bin/bash

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

if [ "x${COMPILER?}" == "xclang" ]; then
  # ... these are the common toolchain utilities for clang and llvm ...
  sudo add-apt-repository -y "deb http://llvm.org/apt/precise/ llvm-toolchain-precise main"

  # ... here is the version of clang ...
  sudo add-apt-repository -y "deb http://llvm.org/apt/precise/ llvm-toolchain-precise${VERSION} main"

  # ... we need to install the public GPG keys from the llvm.org folks ...
  wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
fi

# ... update the apt information after all the modifications are done ...
sudo apt-get -qq update

exit 0
