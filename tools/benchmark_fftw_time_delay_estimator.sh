#!/bin/bash

# Save the stdout and stderr of this script to a file.  Always include
# the raw output of this program in any results.  To summarize in
# Markdown format this might be handy:
#   grep ' summary' bench.log | sed -e 's/summary/|/' -e 's/,/|/g' \
#       -e 's/N=//' -e 's/min=//' -e 's/max=//' -e 's/p[0-9\.]*=//g'

# To run the benchmarks you might want to setup separate build
# directories for each compiler:
#
# cd jaybeams # wherever you checked out the source code
# ./bootstrap
#
# mkdir gcc && cd gcc && CXX=g++ CXXFLAGS="-O3" ../configure
# make -j 4
# ../tools/benchmark_fftw_time_delay_estimator.sh
# cd ..
#
# mkdir clang && cd clang && CXX=clang++ CXXFLAGS="-O3" ../configure
# make -j 4
# ../tools/benchmark_fftw_time_delay_estimator.sh
# cd ..

# ... exit on the first error, print out what commands we will run ...
set -ev

# ... find the common functions for benchmarks and load them ...
bindir=$(dirname $0)
source ${bindir?}/benchmark_common.sh

# ... pick a log file ...
bechmark_startup

# ... run the benchmark for different test cases ...
for test in double:aligned float:aligned double:unaligned float:unaligned; do
    load=$(uptime)
    echo "Running testcase ${test}, current load $load"
    echo | log $LOG
    echo "Running testcase ${test}, current load $load" | log $LOG
    /usr/bin/time ./jb/fftw/bm_time_delay_estimator --test-case=${test} --iterations=1000000 >$TMPOUT 2>$TMPERR
    log $LOG <$TMPERR
    cat $TMPOUT >>$LOG
done

# ... teardown the benchmark configuration changes ...
benchmark_teardown

# ... print out the conditions under which the test ran ...
print_environment jb/fftw/bm_time_delay_estimator | log $LOG

exit 0
