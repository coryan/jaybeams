#!/bin/bash

set -e

if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" ]; then
    echo "Testing PR, image creation disabled."
    exit 0
fi

#if [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
#    echo "DEBUG: only create images on master branch."
#    exit 0
#fi

if [ -z "${DOCKER_USER?}" ]; then
    echo "DOCKER_USER not set, docker autobuilds disabled."
    exit 0
fi

docker login -u "${DOCKER_USER?}" -p "${DOCKER_PASSWORD?}"

# Extract the variant from the IMAGE environment variable (it is set in .travis.yml)
IMAGE=$(echo ${IMAGE?} | sed  -e 's/:.*//')
variant=$(echo ${IMAGE?} | sed -e 's;coryan/jaybeamsdev-;;')

# ... that determines the name of the image we want to build ...
IMAGE=coryan/jaybeams-runtime-${variant?}

# ... make sure the image is available ...
docker pull ${IMAGE?}:latest

# Determine now old is the image, if it is old enough, we re-create
# from scratch every time ...
now=$(date +%s)
image_creation=$(date --date=$(docker inspect -f '{{ .Created }}' ${IMAGE?}:latest) +%s)
age_days=$(( (now - image_creation) / 86400 ))

# By default we reuse the source image as a cache.  This is why we do
# not use --squash btw, it would remove the opportunity for caching ...
caching="--cache-from=${IMAGE?}:latest"
if [ ${age_days?} -ge 30 ]; then
    caching="--no-cache"
fi

# Build a new docker image
echo PWD = $PWD
ls -ld build/staging
cp docker/runtime/${variant?}/Dockerfile build/staging
docker image build ${caching?} -t ${IMAGE?}:tip build/staging

# Compare the ids of the :tip and :latest ...
id_tip=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:tip)
id_latest=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:latest)
if [ ${id_tip?} != ${id_latest?} ]; then
    # They are different, we need to push them.  Create a permanent
    # label so we can keep a history of used images in the registry
    tag=$(date +%Y%m%d%H%M)
    echo "${IMAGE?} has changed, pushing to registry."
    echo "tip    = ${id_tip?}"
    echo "latest = ${id_latest?}"
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
