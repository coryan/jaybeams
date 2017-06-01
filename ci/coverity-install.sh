#!/bin/sh

set -e

wget -q https://scan.coverity.com/download/linux64 \
     --post-data "token=${COVERITY_TOKEN?}&project=jaybeams2&md5=1" \
     -O coverity_tool.md5

test -r coverity_tool.current.md5 || touch coverity_tool.current.md5

if cmp -s coverity_tool.md5 coverity_tool.current.md5; then
    :
else
    wget -q https://scan.coverity.com/download/linux64 \
         --post-data "token=${COVERITY_TOKEN?}&project=jaybeams2" \
         -O coverity_tool.tgz
    cp  coverity_tool.md5 coverity_tool.current.md5
fi

tar xf coverity_tool.tgz

exit 0

