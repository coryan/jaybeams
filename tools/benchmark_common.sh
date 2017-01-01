#!/usr/bin/env

log() {
    awk '{print "#", $0}' >>$1
}

print_header() {
    timestamp=`date -u`
    cat <<EOF
Starting $basename at $timestamp
This file contains both environmental conditions and results
The environmental conditions are printed on lines starting with #
The results are on plain lines without a comment prefix
EOF
}

# Print out a header explaining the file contents
startup() {
    basename=`basename $0 .sh`
    LOG=$basename.$$.results.csv
    TMPERR=/tmp/${basename}.$$.err
    TMPOUT=/tmp/${basename}.$$.out
    trap "/bin/rm -f $TMPERR $TMPOUT" 0
    #LOG=/dev/stdout

    echo "Saving results to $LOG"
    echo '#' >$LOG
    print_header | log $LOG
}

# Prepare the CPU power governor to optimize for performance, this
# tends to produce more consistent results, in other settings the CPU
# often stays in low power settings for too long and the benchmark
# runs slower than expected.
setup_cpu_governor() {
    # ... preserve the current CPU power management settings ...
    governor=`cpupower frequency-info -p | grep 'The governor ".*" may decide' | cut -f2,2 -d'"' `
    # ... restore the cpu power governor at the end of the benchmark ...
    trap "/bin/rm -f $TMPERR $TMPOUT; sudo cpupower frequency-set -g $governor" 0
    # ... change the cpu power governor ...
    echo
    echo "Change the CPU power management governor to 'performance':"
    sudo cpupower frequency-set -g performance
    # ... print out the settings after the change
    echo
    echo "Current CPU settings:"
    sudo cpupower frequency-info
}

# Print out the relevant (and some potentially irrelevant)
# environmental conditions
print_environment() {
    # capture the name of the program

    program=$1
    # ... print out the current git revision ...
    git rev-parse HEAD

    # ... print out compilation details ...
    cat jb/testing/compile_info.cpp || \
        echo "cannot find compile_info.cpp file"

    # ... print out the libraries linked against the program ...
    echo "ldd version"
    ldd $1

    # ... print out the C library information ...
    echo "ldd version"
    ldd --version || echo "cannot run ldd to get libc info"

    # ... another way to print the C library info ...
    echo "glibc version"
    `ldd /usr/bin/touch | grep libc.so | awk '{print $3}'` --version || \
        echo "failed to get libc.so information"

    # ... print out the available memory ...
    echo "Memory and swap information"
    free

    # ... print out how many CPUs (or cores or whatevers) ...
    echo "Processor count"
    egrep ^processor /proc/cpuinfo 

    # ... print out the details of hardware, including CPU, memory, etc ...
    echo "Hardware details:"
    sudo lshw -sanitize
}

# Print out the summary results in Markdown format
summary_as_markdown() {
    cat <<EOF
| test case | size | min | p25 | p50 | p75 | p90 | p99 | p99.9 | max |  N |
|-----------|------|-----|-----|-----|-----|-----|-----|-------|-----|----|
EOF
    if [ $LOG != "/dev/stdout" ]; then
        grep ' summary' $LOG | \
            sed -e 's/^# /| /' \
                -e 's/ summary size=/ | /' \
                -e 's/ summary / | /g' \
                -e 's/ min=/ | /g' \
                -e 's/us, max=/ | /g' \
                -e 's/us, p[0-9\.]*=/ | /g' \
                -e 's/us, N=/ | /g' | \
            awk '{print $0, "|"}'
    fi
}
