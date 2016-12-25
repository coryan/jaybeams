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

# ... get the current CPU power management settings ...
governor=`cpupower frequency-info -p | grep 'The governor ".*" may decide' | cut -f2,2 -d'"' `

# ... set the CPU power management to optimize for performance ...
sudo cpupower frequency-set -g performance

# ... print out the CPU power settings in case they did not have the
# intended effect ...
sudo cpupower frequency-info

# ... make sure we reset the 
trap "sudo cpupower frequency-set -g $governor" 0

# ... run different iterations of the benchmark ...
for test in double:aligned float:aligned double:unaligned float:unaligned; do
    echo ---
    # ... print out the uptime before running each micro-benchmark ...
    /usr/bin/uptime
    /usr/bin/time ./jb/fftw/bm_time_delay_estimator --test-case=${test} --iterations=1000000
done

# ... print out the current git revision ...
git rev-parse HEAD

# ... print out compilation details ...
cat jb/testing/compile_info.cpp || echo "cannot find file"

# ... print out the libraries linked against the program ...
ldd jb/fftw/bm_time_delay_estimator

# ... print out the C library information ...
ldd --version || echo "cannot run ldd to get libc info"

# ... another way to print the C library info ...
`ldd /usr/bin/touch | grep libc.so | awk '{print $3}'` --version || \
    echo "failed to get libc.so information"

# ... print out the available memory ...
free

# ... print out how many CPUs (or cores or whatevers) ...
egrep ^processor /proc/cpuinfo 

# ... print out the details of hardware, including CPU, memory, etc ...
sudo lshw -sanitize

exit 0
