#!/bin/sh

# ... exit on the first error, print out what commands we will run ...
set -e

echo "This script may prompt you for your sudo password"

echo "Resetting CPU power to 'ondemand'"
sudo cpupower frequency-set -g ondemand

echo "Resetting RT scheduling limits to 95% of the CPU time"
echo 950000 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

exit 0
