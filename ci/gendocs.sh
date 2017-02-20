#!/bin/bash

set -e

# Get the original repository, so we can push to the right repository when needed
REPO=$(git config remote.origin.url)
REPO_REF=${REPO/https:\/\/}

if [ "x$TRAVIS_PULL_REQUEST" != "xfalse" -o "x${GENDOCS}" != "xyes" ]; then
    echo "Build is a pull request or GENDOCS is not 'yes', aborting"
    exit 0
fi

if [ "x${TRAVIS_BRANCH}" != "xmaster" ]; then
    echo "Building branch ${TRAVIS_BRANCH}, documentation only generated from 'master'"
    exit 0
fi

docker run --rm -it --env GIT_NAME="${GIT_NAME?}" --env GIT_EMAIL=${GIT_EMAIL?} \
       -v $PWD:/home/${USER?}/jaybeams --workdir /home/${USER?}/jaybeams \
       ${IMAGE?} ci/gendocs.sh

cd doc/html
git push https://${GH_TOKEN?}@${REPO_REF?} gh-pages

exit 0
