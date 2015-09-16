#!/bin/bash
#
# Typically this script is sourced to setup the CC and CXX environment
# variables.

# ... set the CXX and CC variables based on COMPILER and VERSION
if [ "x${COMPILER?}" == "xgcc" ]; then
    CXX=g++${VERSION}
    CC=gcc${VERSION}
elif [ "x${COMPILER?}" == "xclang" ]; then
    CXX=clang++${VERSION}
    CC=clang${VERSION}
else
    echo "Unknown compiler ${COMPILER?}"
    exit 1
fi

export CXX
export CC

# ... set the CXXFLAGS based on the VARIANT ...
if [ "x${VARIANT?}" == "xopt" ]; then
    CXXFLAGS="-O3"
elif [ "x${VARIANT?}" == "xcov" ]; then
    CXXFLAGS="-O0 -g -coverage"
elif [ "x${VARIANT?}" == "xdbg" ]; then
    CXXFLAGS="-O0 -g"
elif [ "x${VARIANT?}" == "xasan" ]; then
    CXXFLAGS="-O2 -g -fsanitize=address"
else
    echo "Unknown variant ${VARIANT?}"
    exit 1
fi

export CXXFLAGS

# ... disable document generation for all branches except 'master' and
# for pull request builds ...
if [ "x${GENDOCS}" == "xyes" ]; then
    # ... temporarily enable wrap-fftw to test how it works ..
    if [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
        GENDOCS=disabled
    fi
    if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" ]; then
        GENDOCS=disabled
    fi
fi

LD_LIBRARY_PATH=$HOME/deps/clFFT-2.6.1-Linux-x64/lib64:${LD_LIBRARY_PATH}
export LD_LIBRARY_PATH
CLFFT_CPPFLAGS=-I$HOME/deps/clFFT-2.6.1-Linux-x64/include
export CLFFT_CPPFLAGS
CLFFT_LDFLAGS=-L$HOME/deps/clFFT-2.6.1-Linux-x64/lib64
export CLFFT_LDFLAGS


