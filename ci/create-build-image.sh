#!/bin/sh

set -e

# TODO() add -a "x${TRAVIS_BRANCH}" = "xmaster"
if [ "x${TRAVIS_PULL_REQUEST}" = "xfalse" ]; then
    exit 0
fi

# TODO() this is just to run less builds during debugging ...
if [ "x${TRAVIS_BRANCH}" != "xcreate-docker-on-travis" -o \
			 "$IMAGE" != "coryan/jaybeamsdev-fedora25" ]; then
    exit 0
fi

# Extract the variant from the IMAGE environment variable (it is set in .travis.yml)
variant=$(echo ${IMAGE?} | sed -e 's;coryan/jaybeamsdev-;;' -e 's/:.*//')

# Build a new docker image, reusing the source image as a cache.  This
# is why we do not use --squash btw, it would remove the opportunity
# for caching ...
docker image build --cache-from=${IMAGE?} -t coryan/jaybeamsdev-${variant?}:tip \
       -f ${variant?}/Dockerfile  docker/dev
