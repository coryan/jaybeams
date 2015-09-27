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
print_environment jb/tde/bm_reduce_argmax_real | log $LOG

# ... capture information about OpenCL devices ...
clinfo | log $LOG

# ... run the benchmark at different sizes ...
for test in gpu:float cpu:float gpu:complex:float cpu:complex:float; do
    for size in 5000 `seq 10000 10000 100000` \
                    `seq 200000 100000 500000`; do
        load=`uptime`
        echo | log $LOG
        echo "Running test at size=$size, current load $load" | log $LOG
        /usr/bin/time chrt -f 50 ./jb/tde/bm_reduce_argmax_real \
                      --benchmark.verbose=1 \
                      --benchmark.iterations=10000 \
                      --benchmark.prefix="${test?},${size?}," \
                      --benchmark.test-case="${test?}" \
                      --benchmark.size=${size?} >$TMPOUT 2>$TMPERR
        cat $TMPOUT >>$LOG
        cat $TMPERR | log $LOG
    done
done

# ... produce a SVG graph using R ..
echo "Running R script to produce SVG graph"
Rscript $bindir/benchmark_opencl_launch_kernel.R $LOG

# ... if the LOG file was a real log file we can print the summary in
# Markdown format ...
echo "Summary results in Markdown (useful to insert into github issues)"
summary_as_markdown

exit 0
