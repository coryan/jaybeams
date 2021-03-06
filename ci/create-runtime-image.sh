#!/bin/bash

set -e

if [ "x${CREATE_RUNTIME_IMAGE}" != "xyes" ]; then
    echo "Runtime image creation not enabled on this build."
    exit 0
fi

if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" ]; then
    echo "Testing PR, image creation disabled."
    exit 0
fi

if [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
    echo "DEBUG: only create images on master branch."
    exit 0
fi

# ... copy the data from the source image ...
SOURCE="cached-${DISTRO?}-${DISTRO_VERSION?}";
mkdir staging || echo "staging directory already exist"
sudo docker run --volume $PWD/staging:/d --rm -it ${SOURCE}:tip cp -r /usr/local /d;

# ... make sure the image is available ...
variant=${DISTRO?}${DISTRO_VERSION?}
IMAGE=coryan/jaybeams-runtime-${variant?}
sudo docker pull ${IMAGE?}:latest

# Determine now old is the image, if it is old enough, we re-create
# from scratch every time ...
now=$(date +%s)
image_creation=$(date --date=$(sudo docker inspect -f '{{ .Created }}' ${IMAGE?}:latest) +%s)
age_days=$(( (now - image_creation) / 86400 ))

# By default we reuse the source image as a cache.  This is why we do
# not use --squash btw, it would remove the opportunity for caching ...
caching="--cache-from=${IMAGE?}:latest"
if [ ${age_days?} -ge 30 ]; then
    caching="--no-cache"
fi

# Build a new docker image
cp docker/runtime/${variant?}/Dockerfile staging
sudo docker image build ${caching?} -t ${IMAGE?}:tip staging

if [ -z "${DOCKER_USER}" -o -z "${DOCKER_PASSWORD}" ]; then
    echo "DOCKER_USER / DOCKER_PASSWORD not set, docker push disabled."
    exit 0
fi

sudo docker login -u "${DOCKER_USER?}" -p "${DOCKER_PASSWORD?}"

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
    sudo docker image tag ${IMAGE?}:tip ${IMAGE?}:${tag?}
    # ... upload the image with that tag ...
    sudo docker image push ${IMAGE?}:${tag?}
    # ... if that succeeds then rename :latest and push it.  The
    # second push should take almost no time, as the layers should all
    # be uploaded already ...
    sudo docker image tag ${IMAGE?}:${tag?} ${IMAGE?}:latest
    sudo docker image push ${IMAGE?}:latest
else
    echo "No changes in ${IMAGE?}, not pushing to registry."
fi

exit 0
