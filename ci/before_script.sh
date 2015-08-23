#!/bin/sh
#
# Typically this script is sourced to setup the CC and CXX environment
# variables.

# exit on error
set -e

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

if [ "x${VARIANT?}" == "xopt" ]; then
    CXXFLAGS="-O3"
elif [ "x${VARIANT?}" == "xcov" ]; then
    CXXFLAGS="-O0 -g -coverage"
elif [ "x${VARIANT?}" == "xdbg" ]; then
    CXXFLAGS="-O0 -g"
else
    echo "Unknown variant ${VARIANT?}"
    exit 1
fi

export CXXFLAGS
