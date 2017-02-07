#!/bin/bash

set -e

env

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

# Extract the variant from the IMAGE environment variable (it is set in .travis.yml)
IMAGE=$(echo ${IMAGE?} | sed  -e 's/:.*//')
variant=$(echo ${IMAGE?} | sed -e 's;coryan/jaybeamsdev-;;')

# Determine now old is the image, if it is old enough, we re-create
# from scratch every time ...
now=$(date +%s)
image_creation=$(date --date=$(docker inspect -f '{{ .Created }}' ${IMAGE}:latest) +%s)
age_days=$(( (now - image_creation) / 86400 ))

# By default we reuse the source image as a cache.  This is why we do
# not use --squash btw, it would remove the opportunity for caching ...
caching="--cache-from=${IMAGE?}:latest"
if [ ${age_days?} -ge 30 ]; then
    caching="--no-cache"
fi

# Build a new docker image
docker image build ${caching?} -t ${IMAGE?}:tip \
       -f docker/dev/${variant?}/Dockerfile docker/dev

# Compare the ids of the :tip and :latest ...
id_tip=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:tip)
id_latest=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:latest)
if [ ${id_tip?} != ${id_latest?} ]; then
    # They are different, we need to push them.  Create a permanent
    # label so we can keep a history of used images in the registry
    tag=$(date +%Y%m%d%H%M)
    echo "${IMAGE?} has changed, pushing to registry."
    # ... label the image with the new tag ...
    docker image tag ${IMAGE?}:tip ${IMAGE?}:${tag?}
    # ... upload the image with that tag ...
    docker image push ${IMAGE?}:${tag?}
    # ... if that succeeds then rename :latest and push it.  The
    # second push should take almost no time, as the layers should all
    # be uploaded already ...
    docker image tag ${IMAGE?}:${tag?} ${IMAGE?}:latest
    docker image push ${IMAGE?}:latest
else
    echo "No changes in ${IMAGE?}, not pushing to registry."
fi

exit 0
