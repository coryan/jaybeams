#!/bin/sh

set -e

cd ${TRAVIS_BUILD_DIR?}
test -d coverity || mkdir coverity
cd coverity

wget -q https://scan.coverity.com/download/linux64 \
     --post-data "token=${COVERITY_TOKEN?}&project=jaybeams2&md5=1" \
     -O coverity_tool.md5

test -r coverity_tool.current.md5 || touch coverity_tool.current.md5

if cmp -s coverity_tool.md5 coverity_tool.current.md5; then
    echo "Coverity Scan is up to date, no download required."
else
    echo "Downloading Coverity Scan..."
    wget -q https://scan.coverity.com/download/linux64 \
         --post-data "token=${COVERITY_TOKEN?}&project=jaybeams2" \
         -O coverity_tool.tgz && \
        tar xf coverity_tool.tgz && \
        cp  coverity_tool.md5 coverity_tool.current.md5
    echo "   done."
fi

exit 0
