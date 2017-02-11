#!/bin/sh

if [ "x${CHECK_STYLE}" != "xyes" ]; then
    echo "This build does not check formatting, \$CHECK_STYLE != 'yes'"
    exit 0
fi

exec docker run --rm -it --env CXX=${COMPILER?} --env CXXFLAGS="${CXXFLAGS}" \
     -v $PWD:$PWD --workdir $PWD ${IMAGE?} make check-style
