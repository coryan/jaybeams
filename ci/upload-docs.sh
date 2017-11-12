#!/usr/bin/env bash

set -e

# Get the original repository, so we can push to the right repository when needed
REPO=$(git config remote.origin.url)
REPO_REF=${REPO/https:\/\/}

if [ "x${TRAVIS_PULL_REQUEST}" != "xfalse" ]; then
    echo "Document generation disabled in pull requests"
    exit 0
fi

if [ "x${BUILD_EXTRA}" != "xGENDOCS" ]; then
    echo "Document generation not enabled"
    exit 0
fi

if [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
    echo "Building branch ${TRAVIS_BRANCH}, documentation only generated from 'master'"
    exit 0
fi

# ... configure git to use the username and address setup from Travis ...
git config --global user.name "${GIT_NAME?}"
git config --global user.email "${GIT_EMAIL?}"

# ... get the repo url ...
REPO_URL=$(git config --get remote.origin.url)

# ... clone the repo into the doc/html subdirectory ...
git clone "${REPO_URL?}" doc/html

# ... then checkout the gh-pages branch, but remove any previous content ...
(cd doc/html && git checkout gh-pages && git rm -qfr .)

# ... finally copy the documents generated during the build into that checkout ...
IMAGE="cached-${DISTRO?}-${DISTRO_VERSION?}";
sudo docker run --volume $PWD/doc:/d --rm -it ${IMAGE}:tip cp -r /var/tmp/build-jaybeams/doc/html /d;

# ... at this point the doc/html subdirectory contains a fresh copy of the generated documents.  git will detect what
# changed and only push those changes to the gh-pages branch ...
cd doc/html
git add --all .
git commit -q -m"Automatically generated documentation" || exit 0
git push https://${GH_TOKEN?}@${REPO_REF?} gh-pages

exit 0
