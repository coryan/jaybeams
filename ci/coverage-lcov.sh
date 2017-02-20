#!/bin/sh

# Get all the converage information
lcov --directory . --capture --output-file coverage.info

# Remove coverage information about template code in system libraries,
# as well as any coverage information for unit tests.
lcov --remove coverage.info \
     '/usr/include/*' '/usr/local/*' \
     'jb/ut_*' 'jb/*/ut_*' 'jb/testing/check_*' \
     --output-file coverage.info

# Show the code coverage results in the Travis CI log
lcov --list coverage.info

exit 0


