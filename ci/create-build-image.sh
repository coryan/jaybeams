#!/bin/bash

set -e

# Extract the variant from the IMAGE environment variable (it is set
# in .travis.yml) ...
IMAGE=${IMAGE/:*//}
variant=${IMAGE#coryan/jaybeamsdev-}

# ... determine now old is the image, if it is old enough, we
# re-create from scratch every time ...
now=$(date +%s)
image_creation=$(date --date=$(sudo docker inspect -f '{{ .Created }}' ${IMAGE?}:latest) +%s)
age_days=$(( (now - image_creation) / 86400 ))

# ... by default we reuse the source image as a cache.  This is why we do
# not use --squash btw, it would remove the opportunity for caching ...
caching="--cache-from=${IMAGE?}:latest"
if [ ${age_days?} -ge 30 ]; then
    caching="--no-cache"
fi

# Build a new docker image
sudo docker image build ${caching?} -t ${IMAGE?}:tip \
       -f docker/dev/${variant?}/Dockerfile docker/dev

exit 0
