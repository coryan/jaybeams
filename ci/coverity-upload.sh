#!/bin/bash

set -e

tar zcf jaybeams.tgz -C build cov-int

if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" ]; then
    echo "Testing PR, image creation disabled."
    exit 0
fi

if [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
    echo "DEBUG: only create images on master branch."
    ls -l
    ls -l build/cov-int
    exit 0
fi

curl --form token=${COVERITY_TOKEN?} \
  --form email=coryan@users.noreply.github.com \
  --form file=@jaybeams.tgz \
  --form version=$(git rev-parse --short HEAD) \
  --form description="Manual Build" \
  https://scan.coverity.com/builds?project=jaybeams2

exit 0