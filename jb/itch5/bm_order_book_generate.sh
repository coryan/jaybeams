#!/bin/bash

# exit on the first error ...
set -e

iterations=35000
if [ $# -ge 1 ]; then
    iterations=$1
fi

bindir=$(dirname $0)
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

program=$(find_program ${bindir?} bm_order_book)
echo "Driving tests for ${program?}"

# ... configure the system to run a benchmark and select where to send
# output ...
benchmark_startup

# ... print the environment and configuration, we must do this early
# because the sudo credentials expire while the program is running,
# and we do not want to miss this information ...
echo "Capturing system configuration... patience..."
print_environment ${bindir?} ${program?} | log $LOG

# ... create the baseline data for the order book benchmark ...
for test in map array; do
    if which uptime >/dev/null 2>/dev/null; then
	load=$(uptime)
    else
	load=$(/bin/echo -n "(raw) " ; cat /proc/loadavg)
    fi
    echo "Running testcase ${test?}, current load ${load?}"
    echo | log $LOG
    echo "Running testcase ${test?}, current load ${load?}" | log $LOG
    /usr/bin/time ${program?} \
                  --microbenchmark.verbose=true \
                  --microbenchmark.prefix=${test?}, \
                  --microbenchmark.iterations=${iterations?} \
                  --microbenchmark.test-case=${test?} \
		  >$TMPOUT 2>$TMPERR
    log $LOG <${TMPERR}
    cat $TMPOUT >>$LOG
done

# ... teardown the benchmark configuration changes ...
benchmark_teardown

# ... print the summary as a nice markdown table ...
summary_as_markdown

exit 0
