#!/bin/bash

# exit on the first error ...
set -e

iterations=1000
if [ $# -ge 1 ]; then
    iterations=$1
fi

bindir=`dirname $0`
for path in ${bindir?}/../../tools ${bindir?}; do
    if [ -r ${path?}/benchmark_common.sh ]; then
	. ${path?}/benchmark_common.sh
	benchmark_common_loaded=yes
	break
    fi
done
if [ "x${benchmark_common_loaded}" = "x" ]; then
    echo "Cannot load benchmark_common.sh"
    exit 1
fi

# ... configure the system to run a benchmark and select where to send
# output ...
benchmark_startup

# ... print the environment and configuration, we must do this early
# because the sudo credentials expire while the program is running,
# and we do not want to miss this information ...
echo "Capturing system configuration... patience..."
print_environment jb/itch5/bm_order_book | log $LOG

# ... create the baseline data for the order book benchmark ...
for test in map array; do
    load=`uptime`
    echo "Running testcase ${test?}, current load ${load?}"
    echo | log $LOG
    echo "Running testcase ${test?}, current load ${load?}" | log $LOG
    /usr/bin/time ./jb/itch5/bm_order_book \
                  --microbenchmark.verbose=true \
                  --microbenchmark.prefix=${test?}, \
                  --microbenchmark.iterations=${iterations?} \
                  --microbenchmark.test-case=${test?} \
		  >$TMPOUT 2>$TMPERR
    cat $TMPERR | log $LOG
    cat $TMPOUT >>$LOG
done

# ... teardown the benchmark configuration changes ...
benchmark_teardown

# ... print the summary as a nice markdown table ...
summary_as_markdown

exit 0
