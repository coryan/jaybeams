#!/usr/bin/env bash

set -e

if [ "x${CREATE_ANALYSIS_IMAGE}" != "xyes" ]; then
    echo "Analysis image creation not enabled on this build."
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

if [ "${DISTRO?}" != "ubuntu" -o "${DISTRO_VERSION}" != "16.04" ]; then
    echo "We only need to create the analysis image for Ubuntu 16.04"
    exit 0
fi

# ... copy the data from the source image ...
IMAGE="cached-${DISTRO?}-${DISTRO_VERSION?}";
sudo docker run --volume $PWD/staging:/d --rm -it ${IMAGE}:tip cp -r /usr/local /d;

# ... that determines the name of the image we want to build ...
IMAGE=coryan/jaybeams-analysis

# ... make sure the image is available ...
sudo docker pull ${IMAGE?}:latest

# ... determine now old is the image, if it is old enough, we re-create
# from scratch every time ...
now=$(date +%s)
image_creation=$(date --date=$(sudo docker inspect -f '{{ .Created }}' ${IMAGE?}:latest) +%s)
age_days=$(( (now - image_creation) / 86400 ))

# ... by default we reuse the source image as a cache.  This is why we do
# not use --squash btw, it would remove the opportunity for caching ...
caching="--cache-from=${IMAGE?}:latest"
if [ ${age_days?} -ge 30 ]; then
    caching="--no-cache"
fi

# ... build a new docker image ..
cp docker/analysis/Dockerfile build/staging/Dockerfile.analysis
sudo docker image build ${caching?} -t ${IMAGE?}:tip \
       -f build/staging/Dockerfile.analysis build/staging

if [ -z "${DOCKER_USER}" -o -z "${DOCKER_PASSWORD}" ]; then
    echo "DOCKER_USER / DOCKER_PASSWORD not set, docker push disabled."
    exit 0
fi

sudo docker login -u "${DOCKER_USER?}" -p "${DOCKER_PASSWORD?}"

# ... compare the ids of the :tip and :latest ...
id_tip=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:tip)
id_latest=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:latest)
if [ ${id_tip?} != ${id_latest?} ]; then
    # ... they are different, we need to push them.  Create a
    # permanent label so we can keep a history of used images in the
    # registry ...
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
