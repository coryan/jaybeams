#!/usr/bin/env Rscript
require(ggplot2)

# Read the command-line arguments, 
args <- commandArgs(trailingOnly=TRUE)

# read the given file, skipping comments, into a data.frame()
data <- read.csv(
  args[1], header=FALSE, col.names=c('testcase', 'size', 'nanoseconds'),
  comment.char='#')

# compute the cost per OpenCL kernel launch
data$cost <- data$nanoseconds / data$size / 1000.0

# express the time in microseconds
data$microseconds <- data$nanoseconds / 1000.0

# turn the size (number of OpenCL kernel launches) into a factor so
# each gets a different error bar.
data$fsize <- factor(data$size)

# plot the data
ggplot(data=data, aes(x=fsize, y=cost)) + geom_boxplot() +
  ylab("Microseconds per kernel launch (log scale)") +
  xlab("Number of kernels launched (pipelined)") +
  scale_y_log10()

# save the plot
ggsave(filename="benchmark_opencl_launch_kernel.boxplot.svg",
       width=8.0, height=8.0/1.61)
ggsave(filename="benchmark_opencl_launch_kernel.boxplot.png",
       width=8.0, height=8.0/1.61)

# plot the raw data with a regression
ggplot(data=data, aes(x=size, y=microseconds)) +
  geom_point(alpha=0.05) +
  geom_smooth(method='lm') +
  ylab("Microeconds to launch all kernels") +
  xlab("Number of kernels launched (pipelined)")
ggsave(filename="benchmark_opencl_launch_kernel.fit.svg",
       width=8.0, height=8.0/1.61)
ggsave(filename="benchmark_opencl_launch_kernel.fit.png",
       width=8.0, height=8.0/1.61)

# Fit the data using a linear regression
fit <- lm(nanoseconds ~ size, data)
summary(fit)
print(fit)

q(save="no")
