#!/usr/bin/env Rscript
require(ggplot2)

# Read the command-line arguments, 
args <- commandArgs(trailingOnly=TRUE)

# read the given file, skipping comments, into a data.frame()
data <- read.csv(
  args[1], header=FALSE, col.names=c('testcase', 'size', 'nanoseconds'),
  comment.char='#')

# compute the cost per OpenCL kernel launch
data$cost <- data$nanoseconds / data$size

# turn the size (number of OpenCL kernel launches) into a factor so
# each gets a different error bar.
data$fsize <- factor(data$size)

# plot the data
ggplot(data=data, aes(x=fsize, y=cost)) + geom_boxplot() +
  ylab("nanoseconds per kernel launch") +
  xlab("number of kernels launched (pipelined)")

# save the plot
ggsave(filename="benchmark_opencl_launch_kernel.svg",
       width=8.0, height=8.0/1.61);

q(save="no")
