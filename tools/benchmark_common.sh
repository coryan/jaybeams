#!/usr/bin/env

log() {
    awk '{print "#", $0}' >>$1
}

find_program() {
    bindir=$1
    bin=$2
    program=${bindir?}/${bin?}
    if [ -x ${program?}  ]; then
	echo $program
	return 0
    fi
    program=$(find ./jb -name ${bin})
    if [ "x${program}" != "x" -a -x ${program} ]; then
	echo $program
	return 0
    fi
    echo "Cannot find program ${bin} in the typical locations, aborting"
    exit 1
}

print_header() {
    timestamp=`date -u`
    cat <<EOF
Starting $basename at $timestamp
This file contains both environmental conditions and results
The environmental conditions are printed on lines starting with #
The results are on plain lines without a comment prefix

Most of the system configuration is logged at the end of the results

EOF
}

# Print out a header explaining the file contents
benchmark_startup() {
    cat <<EOF
This script may prompt you for your sudo password.  sudo privileges are
required to
  (1) configure the cpu frequency governor
  (2) disable real-time scheduling limits
Use the companion 'benchmark_teardown.sh' script to restore system
settings to typical defaults.  All changes are logged in case you
need to manually restore them.
EOF
    basename=`basename $0 .sh`
    LOG=$basename.$$.results.csv
    TMPERR=/tmp/${basename}.$$.err
    TMPOUT=/tmp/${basename}.$$.out
    trap "/bin/rm -f $TMPERR $TMPOUT" 0
    #LOG=/dev/stdout

    echo
    echo "Saving results to $LOG"
    echo
    echo '#' >$LOG
    print_header | log $LOG
    cpu_governor_startup | log $LOG
#    full_rt_scheduling_startup | log $LOG
}

benchmark_teardown() {
    full_rt_scheduling_teardown | log $LOG
    cpu_governor_teardown | log $LOG
}

# Prepare the CPU power governor to optimize for performance, this
# tends to produce more consistent results, in other settings the CPU
# often stays in low power settings for too long and the benchmark
# runs slower than expected.
cpu_governor_startup() {
    # ... first find out if there is a cpupower command at all ...
    if which cpupower>/dev/null 2>/dev/null && cpupower frequency-info | grep -q ' driver: '; then
	HAS_CPUPOWER=yes
    else
	HAS_CPUPOWER=no
	echo "WARNING: No cpupower(1) command found or no cpupower driver."
	echo "WARNING: Benchmark will execute without changing the cpupower configuration."
        echo "WARNING: This can result in unpredictable output"
	return 0
    fi
    # ... preserve the current CPU power management settings ...
    old_governor=`cpupower frequency-info -p | grep 'The governor ".*" may decide' | cut -f2,2 -d'"' `
    # ... restore the cpu power governor at the end of the benchmark ...
    trap "/bin/rm -f $TMPERR $TMPOUT; cpu_governor_teardown $old_governor" 0
    # ... change the cpu power governor ...
    echo
    echo "Change the CPU power management governor to 'performance':"
    sudo cpupower frequency-set -g performance
    # ... print out the settings after the change
    echo
    echo "Current CPU settings:"
    sudo cpupower frequency-info
}

cpu_governor_teardown() {
    if [ "x$HAS_CPUPOWER" = "xyes" ]; then
	if [ "x$old_governor" = "x" ]; then
	    gov=$1
	else
	    gov=$old_governor
	fi
	if [ "x$gov" == "x" ]; then
	    echo "no governor variable set, assuming 'ondemand'"
	    gov="ondemand"
	fi
	echo "Restoring cpupower governor to $gov"
	sudo cpupower frequency-set -g $gov
    fi
}

# Print out the relevant (and some potentially irrelevant)
# environmental conditions
print_environment() {
    # capture the directory where the driver script is running from
    bindir=$1
    # capture the name of the program
    program=$2

    # ... in general this prints out metadata about the test from the
    # high-level stuff down towards the hardware ...
    if which git>/dev/null 2>/dev/null; then
	# ... if git is available, print out the current git revision ...
	echo
	echo
	echo "Last revision commited to git"
	git rev-parse HEAD

	echo
	echo "Current git status"
	git status
    fi

    # ... print out compilation details ...
    cmd=$(find_program ${bindir?} show_compile_info)
    echo
    ${cmd?}

    # ... print out the libraries linked against the program ...
    echo
    echo "dynamically loaded libraries in the program"
    ldd ${program?}

    # ... print out the rpms that provided these libraries ...
    if which rpm>/dev/null 2>/dev/null; then
        echo
        echo "dynamic libraries provided by the following packages"
        for lib in $(ldd ${program?} | grep '=>' | grep -v linux-vdso | awk '{print $3}') \
                   $(ldd ${program?} | grep ld-linux | grep -v linux-vdso | awk '{print $1}'); do
            echo ${lib?} ': ' $(rpm -q --whatprovides ${lib?})
        done
    elif which dpkg >/dev/null 2>/dev/null; then
        echo
        echo "dynamic libraries provided by the following packages"
        for lib in $(ldd $1 | grep '=>' | grep -v linux-vdso | awk '{print $3}') \
                   $(ldd $1 | grep ld-linux | grep -v linux-vdso | awk '{print $1}'); do
            echo ${lib?} ': ' $(dpkg -S ${lib?})
            pkg=$(dpkg -S ${lib?} | cut -d: -f1)
            echo ${lib?} ': ' ${pkg?} $(apt-cache show ${pkg?} | grep Version:)
        done
    else
        echo
        echo "cannot determine packages, please report this as a bug"
    fi

    # ... print out the C++ library information ...
    echo
    echo "libstdc++ version"
    lib=$(ldd ${program} | grep libstdc++.so | awk '{print $3}' 2>&1)
    if [ "x${lib?}" = "x" ]; then
        echo "  not using libstdc++"
    elif which strings >/dev/null 2>/dev/null; then
        strings ${lib?} | egrep '(GLIBCXX|GLIBCPP)_[0-9]' | tail -2
    else
	echo "  cannot determine libstdc++ version"
    fi

    # ... print out the C library information ...
    echo
    echo "glibc version via ldd version"
    ldd --version || echo "cannot run ldd to get libc info"

    # ... another way to print the C library info ...
    echo
    echo "glibc version via libc.so --version"
    $(ldd /usr/bin/env | grep libc.so | awk '{print $3}') --version || \
        echo "failed to get libc.so information"

    # ... print out the kernel version and other details, skip the
    # hostname though ....
    echo
    echo "kernel version and platform"
    uname -s -r -v -m -p -i -o

    # ... print out the available memory ...
    if which free >/dev/null 2>/dev/null; then
	echo
	echo "Memory and swap information"
	free
    fi

    # ... print out how many CPUs (or cores or whatevers) ...
    echo
    echo "Processor count"
    egrep ^processor /proc/cpuinfo 

    # ... print out the details of hardware, including CPU, memory, etc ...
    if which lshw >/dev/null 2>/dev/null; then
	echo
	echo "Hardware details:"
	sudo lshw -sanitize
    fi
}

# Print out the summary results in Markdown format
summary_as_markdown() {
    if which awk >/dev/null 2>/dev/null; then
	/bin/true
    else
	return
    fi
    if which sed >/dev/null 2>/dev/null; then
	/bin/true
    else
	return
    fi
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

require_full_rt_scheduling() {
    if cat /proc/sys/kernel/sched_rt_runtime_us | grep -q -- -1; then
	echo "System seems to be configured for benchmarking, good"
    else
	echo "System does not seem to be configured for benchmarking, aborting"
	exit 1
    fi
}

full_rt_scheduling_startup() {
    echo "Disabling RT scheduling limits, RT processes can take 100% of the CPU"
    echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us > /dev/null 2>/dev/null
}

full_rt_scheduling_teardown() {
    echo "Restore RT scheduling limits, RT processses limited to 95% of the CPU"
    echo 950000 | sudo tee /proc/sys/kernel/sched_rt_runtime_us >/dev/null 2>/dev/null
}
