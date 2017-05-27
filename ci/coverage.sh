#!/bin/bash

# Exit on the first error
set -e

if [ "x$TRAVIS_PULL_REQUEST" != "xfalse" ]; then
    echo "Code coverage disabled in pull requests"
    exit 0
fi

if [ "x${COVERAGE}" != "xyes" ]; then
    echo "Code coverage not enabled, this is not a code coverage build"
    exit 0
fi

docker run --rm -it -v $PWD:$PWD --workdir $PWD ${IMAGE?} ci/coverage-lcov.sh
# Push results to coveralls ...
coveralls-lcov --repo-token ${COVERALLS_TOKEN?} coverage.info

if [ "x${CODECOV_TOKEN}" != "x" ]; then
    bash <(curl -s https://codecov.io/bash)
fi

exit 0
