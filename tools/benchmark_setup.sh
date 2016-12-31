#!/bin/sh

# ... exit on the first error, print out what commands we will run ...
set -e

echo "This script may prompt you for your sudo password"

echo "Setting CPU frequency governor to 'performance'"
sudo cpupower frequency-set -g performance

echo "Disabling RT scheduling limits, RT processes can take 100% of the CPU"
echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

exit 0
