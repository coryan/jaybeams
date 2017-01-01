#!/bin/bash

# exit on the first error ...
set -e

bindir=`dirname $0`
. ${bindir?}/../../tools/benchmark_common.sh

# ... configure the system to run a benchmark and select where to send output ...
benchmark_startup

# ... create the baseline data for the order book benchmark ...
for test in map:buy map:sell array:buy array:sell; do
    load=`uptime`
    echo "Running testcase ${test}, current load $load"
    echo "Running testcase ${test}, current load $load" | log $LOG
    /usr/bin/time ./jb/itch5/bm_order_book --seed=3966899719 \
                  --microbenchmark.verbose=true \
                  --microbenchmark.prefix=${test?}, \
                  --microbenchmark.iterations=5000 \
                  --microbenchmark.test-case=${test?} \
		  >$TMPOUT 2>$TMPERR
    cat $TMPOUT >>$LOG
    cat $TMPERR | log $LOG
done

# ... teardown the benchmark configuration changes ...
benchmark_teardown

# ... print the environment and configuration at the end because it is very slow ...
echo "Capturing system configuration... patience..."
print_environment jb/itch5/bm_order_book | log $LOG

# ... print the summary as a nice markdown table ...
summary_as_markdown

exit 0
