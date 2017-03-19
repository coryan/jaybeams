#!/bin/bash

set -e

if [ "x${CREATE_BUILD_IMAGE}" != "xyes" ]; then
    echo "Build image creation not enabled on this build."
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
