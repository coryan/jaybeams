#!/bin/bash

set -e

env

# Extract the variant from the IMAGE environment variable (it is set in .travis.yml)
variant=$(echo ${IMAGE?} | sed -e 's;coryan/jaybeamsdev-;;' -e 's/:.*//')
echo "VARIANT=" ${variant?}

# TODO() add -a "x${TRAVIS_BRANCH}" = "xmaster"
if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" ]; then
    echo "Testing PR, image creation disabled."
    exit 0
fi

# TODO() this is just to run less builds during debugging ...
if [ "x${TRAVIS_BRANCH}" != "xcreate-docker-on-travis" -o \
			 "$IMAGE" != "coryan/jaybeamsdev-fedora25" ]; then
    echo "DEBUG: only branch create-docker-on-travis create images"
    exit 0
fi

# Determine now old is the image, if it is old enough, we re-create
# from scratch every time ...
now=$(date +%s)
image_creation=$(date --date=$(docker inspect -f '{{ .Created }}' ${IMAGE}:latest +%s))
age_days=$(( (now - image_creation) / 86400 ))

# By default we reuse the source image as a cache.  This is why we do
# not use --squash btw, it would remove the opportunity for caching ...
caching="--cache-from=${IMAGE?}:latest"
if [ $age_days -ge 30 ]; then
    caching="--no-cache"
fi

# Build a new docker image
docker image build ${caching?} -t coryan/jaybeamsdev-${variant?}:tip \
       -f ${variant?}/Dockerfile docker/dev

exit 0
