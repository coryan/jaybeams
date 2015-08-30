#!/bin/bash

# Exit on the first error
set -ev

# Get all the converage information
lcov --directory . --capture --output-file coverage.info

# Remove coverage information about template code in system libraries,
# as well as any coverage information for unit tests.
lcov --remove coverage.info \
     '/usr/include/*' '/usr/local/*' \
     'jb/ut_*' 'jb/*/ut_*' \
     --output-file coverage.info

# Show the code coverage results in the Travis CI log
lcov --list coverage.info
coveralls-lcov --repo-token ${COVERALLS_TOKEN?} coverage.info

exit 0
