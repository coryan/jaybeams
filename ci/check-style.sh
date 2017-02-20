#!/bin/sh

if [ "x${CHECK_STYLE}" != "xyes" ]; then
    echo "This build does not check formatting, \$CHECK_STYLE != 'yes'"
    exit 0
fi

exec docker run --rm -it --volume $PWD:$PWD --workdir $PWD/build \
     ${IMAGE?} make check-style
