#!/usr/bin/env Rscript
require(ggplot2)

# Read the command-line arguments, 
args <- commandArgs(trailingOnly=TRUE)

# read the given file, skipping comments, into a data.frame()
data <- read.csv(
  args[1], header=FALSE, col.names=c('testcase', 'size', 'nanoseconds'),
  comment.char='#')

# express the time in microseconds
data$microseconds <- data$nanoseconds / 1000.0

# turn the size (number of OpenCL kernel launches) into a factor so
# each gets a different error bar.
data$fsize <- factor(data$size)

# plot the data
ggplot(data=data, aes(x=fsize, y=microseconds, color=testcase)) +
  geom_boxplot() +
  ylab("Elapsed Time (usecs)") +
  xlab("Vector Size") +
  theme(legend.position="bottom")

# save the plot
ggsave(filename="benchmark_reduce_argmax_real.boxplot.svg",
       width=8.0, height=8.0/1.61)
ggsave(filename="benchmark_reduce_argmax_real.boxplot.png",
       width=8.0, height=8.0/1.61)

# plot the raw data with a regression
ggplot(data=data, aes(x=size, y=microseconds, color=testcase)) +
  geom_point(alpha=0.05) +
  geom_smooth(method='lm') +
  ylab("Elapsed Time (usecs)") +
  xlab("Vector Size")
ggsave(filename="benchmark_reduce_argmax_real.fit.svg",
       width=8.0, height=8.0/1.61)
ggsave(filename="benchmark_reduce_argmax_real.fit.png",
       width=8.0, height=8.0/1.61)

q(save="no")
