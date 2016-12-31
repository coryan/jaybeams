#!/bin/bash

# ... exit on the first error, print out what commands we will run ...
set -e

# ... find the common functions for benchmarks and load them ...
bindir=`dirname $0`
source ${bindir?}/benchmark_common.sh

# ... pick a log file ...
startup

# ... set the CPU power management to optimize for performance ...
setup_cpu_governor | log $LOG

# ... capture the conditions under which the test ran, do that before
# sudo expires ...
print_environment jb/opencl/bm_launch_kernel | log $LOG

# ... capture information about OpenCL devices ...
clinfo | log $LOG

# ... run the benchmark at different sizes ...
for size in `seq 1 10` `seq 20 10 100` `seq 200 100 1000`; do
    load=`uptime`
    echo | log $LOG
    echo "Running test at size=$size, current load $load" | log $LOG
    /usr/bin/time chrt -f 50 ./jb/opencl/bm_launch_kernel \
                  --benchmark.verbose=1 \
                  --benchmark.iterations=10000 \
                  --benchmark.test-case="$size" \
                  --benchmark.prefix="launch_kernel,${size}," \
                  --benchmark.size=${size?} >$TMPOUT 2>$TMPERR
    cat $TMPOUT >>$LOG
    cat $TMPERR | log $LOG
done

# ... produce a SVG graph using R ..
echo "Running R script to produce SVG graph"
Rscript $bindir/benchmark_opencl_launch_kernel.R $LOG

# ... if the LOG file was a real log file we can print the summary in
# Markdown format ...
echo "Summary results in Markdown (useful to insert into github issues)"
summary_as_markdown

exit 0
