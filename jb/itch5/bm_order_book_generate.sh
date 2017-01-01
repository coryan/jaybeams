#!/bin/bash

# exit on the first error ...
set -e

if cat /proc/sys/kernel/sched_rt_runtime_us | grep -q -- -1; then
    echo "System seems to be configured for benchmarking, good"
else
    echo "System does not seem to be configured for benchmarking, aborting"
    exit 1
fi

# ... create the baseline data for the order book benchmark ...
echo "Running ... be patient..."
OUT=bm_order_book_results.$$.csv
LOG=bm_order_book_annotations.$$.log
echo >${OUT?}
for test in map:buy map:sell array:buy array:sell; do
    echo ${test?}
    /usr/bin/time ./jb/itch5/bm_order_book --seed=3966899719 \
                  --microbenchmark.thread.affinity=1,2,3 \
                  --microbenchmark.verbose=true \
                  --microbenchmark.prefix=${test?}, \
                  --microbenchmark.iterations=50000 \
                  --microbenchmark.test-case=${test?} >>${OUT?}
done 2>${LOG?}

echo "Results were saved in ${OUT?}"
echo "you may want to analyze them and if accepted commit them to the repo"
echo "Additional information was generated in ${LOG}"

exit 0
