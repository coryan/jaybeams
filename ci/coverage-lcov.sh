#!/bin/sh

# Get all the converage information
lcov --directory . --capture --output-file coverage.info

# Remove coverage information about template code in system libraries,
# as well as any coverage information for unit tests.
lcov --remove coverage.info \
     'jb/ut_*' \
     'jb/*/ut_*' \
     'jb/testing/check_*' \
     'jb/itch5/testing/*' \
     'valgrind/valgrind.h' \
     '*.pb.h' \
     '*.pb.cc' \
     'ext/googletest/googlemock/*/*' \
     'ext/googletest/googletest/*/*' \
     'ext/*/*' \
     '/usr/include/*' \
     '/usr/local/*' \
     --output-file coverage.info

# Show the code coverage results in the Travis CI log
lcov --list coverage.info

exit 0


