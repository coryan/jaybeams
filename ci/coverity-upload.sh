#!/bin/bash

set -e

if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" -o "x${COVERITY}" != "xyes" ]; then
    echo "Build is a pull request or COVERITY is not 'yes', aborting."
    exit 0
fi

if [ ! -r build/jaybeams-coverity-upload.tgz ]; then
    echo "No coverity output, skipping upload."
    exit 0
fi

curl --form token=${COVERITY_TOKEN?} \
  --form email=coryan@users.noreply.github.com \
  --form file=@build/jaybeams-coverity-upload.tgz \
  --form version=$(git rev-parse --short HEAD) \
  --form description="Travis-CI Build, branch=${TRAVIS_BRANCH}" \
  https://scan.coverity.com/builds?project=jaybeams2

exit 0
