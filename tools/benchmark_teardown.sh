#!/bin/sh

# ... exit on the first error, print out what commands we will run ...
set -e

bindir=$(dirname $0)
. ${bindir?}/benchmark_common.sh
benchmark_teardown

exit 0
