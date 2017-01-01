#!/usr/bin/env Rscript
require(ggplot2)
require(boot)

# Read the command-line arguments, 
args <- commandArgs(trailingOnly=TRUE)

baseline.file <- args[1]
candidate.file <- args[2]

baseline.file <- '/home/coryan/jaybeams/gcc/bm_order_book_results.17896.csv'
candidate.file <- '/home/coryan/jaybeams/gcc/bm_order_book_results.25558.csv'


# read the given files, skipping comments, into a data.frame()
baseline <- read.csv(
  baseline.file, header=FALSE, col.names=c('testcase', 'size', 'nanoseconds'),
  comment.char='#')
baseline$run <- factor('baseline')
candidate <- read.csv(
  candidate.file, header=FALSE, col.names=c('testcase', 'size', 'nanoseconds'),
  comment.char='#')
candidate$run <- factor('candidate')

data <- rbind(baseline, candidate)

# express the time in microseconds
data$microseconds <- data$nanoseconds / 1000.0

# turn the size (number of OpenCL kernel launches) into a factor so
# each gets a different error bar.
data$fsize <- factor(data$size)

data$booktype <- factor(sapply(data$testcase, function(x) strsplit(as.character(x), split=':')[[1]][1]))
data$side <- factor(sapply(data$testcase, function(x) strsplit(as.character(x), split=':')[[1]][2]))

# plot the data, it is pretty obvious that the map book is faster ...
ggplot(data=data, aes(x=testcase, y=microseconds, color=booktype)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Test Case") +
  theme(legend.position="bottom")

ggplot(data=data, aes(x=testcase, y=microseconds, color=booktype)) +
  geom_violin() +
  ylab("Elapsed Time (us)") +
  xlab("Test Case") +
  theme(legend.position="bottom")

ggplot(data=data, aes(x=run, y=microseconds, color=booktype)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

ggplot(data=data, aes(x=run, y=microseconds, color=booktype)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

ggplot(data=data, aes(x=run, y=microseconds, color=testcase)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

ggplot(data=data, aes(x=run, y=microseconds, color=testcase)) +
  geom_violin() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

# ... but compute the statistics anyway ...
aggregate(microseconds ~ booktype, data=data, summary)

# ... and run the statistical test ...
wilcox.test(microseconds ~ booktype, data=data, conf.int=TRUE)

# ... the null hypothesis is that both are the same, if p-value is
# small, we can reject the null hypothesis ...
wilcox.test(microseconds ~ booktype, data=subset(data, run='baseline'), conf.int=TRUE)
wilcox.test(microseconds ~ booktype, data=subset(data, run='candidate'), conf.int=TRUE)

# ... are the two runs very different? ...
aggregate(microseconds ~ booktype + run, data=data, FUN=summary)
aggregate(microseconds ~ run + testcase, data=data, FUN=summary)

# ... try with some stats ...
wilcox.test(microseconds ~ run, data=subset(data, booktype='array'))
wilcox.test(microseconds ~ run, data=subset(data, booktype='map'))
kruskal.test(microseconds ~ run, data=subset(data, booktype='array'))
kruskal.test(microseconds ~ run, data=subset(data, booktype='map'))

aggregate(microseconds ~ booktype + run, data=data, FUN=sd)
aggregate(microseconds ~ booktype + run, data=data, FUN=mean)

# ... let's see if we can expect reproduceable results from multiple
# runs ...

baseline.buy <- subset(data, side == 'buy' & run == 'baseline')
summary(baseline.buy)

b <- boot(baseline.buy$microseconds, function(x,d) sd(x[d]), R=1000)
b
plot(b)

wilcox.test(microseconds ~ run, data=subset(data, booktype=='map'&side=='buy'))

ggplot(data=subset(data, booktype=='map'),
       aes(x=run, y=microseconds, color=side)) +
  geom_violin() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

aggregate(microseconds ~ run + side, data=subset(data, booktype=='map'),FUN=sd)

q(save='no')
