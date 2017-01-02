#!/usr/bin/env Rscript
require(ggplot2)
require(boot)
require(pwr)

## How much data do we need to collect to obtain statiscally
## significant results?  This value was obtained using
## bm_order_book_analyze_initial.R:
min.samples <- 5000
expected.sd <- 200

## Pick your code branch here ...
args <- commandArgs(trailingOnly=TRUE)

baseline.file <- args[1]
candidate.file <- args[2]

## read the given files, skipping comments, into a data.frame()
baseline <- read.csv(
    baseline.file, header=FALSE, col.names=c('testcase', 'nanoseconds'),
    comment.char='#')
baseline$run <- factor('baseline')
candidate <- read.csv(
    candidate.file, header=FALSE, col.names=c('testcase', 'nanoseconds'),
    comment.char='#')
candidate$run <- factor('candidate')
data <- rbind(baseline, candidate)

## express the time in microseconds
data$microseconds <- data$nanoseconds / 1000.0

## extract the booktype and side from the combined testcase column
data$booktype <- factor(sapply(
    data$testcase,
    function(x) strsplit(as.character(x), split=':')[[1]][1]))
data$side <- factor(sapply(
    data$testcase,
    function(x) strsplit(as.character(x), split=':')[[1]][2]))

## Is this data any good?  Does it have the minimum number of samples
## to be sufficiently powered ...
samples.per.factor <- aggregate(
    microseconds ~ run + testcase, data=data,
    FUN=length)
actual.min.samples <- min(samples.per.factor$microseconds)

if (actual.min.samples < min.samples) {
    print(paste0("The test must have at least ", min.samples,
                 " samples.  Only ", actual.min.samples,
                 " present for some testcases"))
    print(samples.per.factor)
    quit(save='ask')
} else {
    print(paste0("The test had enough samples for power=",
                 desired.power, ", significance=", desired.significance))
}

## Is the data on 'candidate' significantly different from the data in 
## 'baseline', first just plot it:

## ... a pretty plot, which is sometimes hard to read ..
ggplot(data=data, aes(x=run, y=microseconds, color=testcase)) +
  geom_violin() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

## ... an easier to read plot ...
ggplot(data=data, aes(x=run, y=microseconds, color=testcase)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Program Run") +
  theme(legend.position="bottom")

## ... and another variation ..
ggplot(data=data, aes(x=testcase, y=microseconds, color=run)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Test Case") +
  theme(legend.position="bottom")

## ... and one more ...
ggplot(data=subset(data, testcase=='array:sell'),
       aes(x=testcase, y=microseconds, color=run)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Test Case") +
  theme(legend.position="bottom")

## ... just print a summary by run and booktype first ...
r.median <- aggregate(microseconds ~ run + testcase, data=data, FUN=median)
summary(r.median)
ggplot(data=r.median, aes(x=testcase, y=microseconds, color=run)) +
  geom_point(size=4) +
  ylab("Elapsed Time (us)") +
  xlab("Test Case") +
  theme(legend.position="bottom")

## ... calculate the range across all runs ...
r.range <- aggregate(
    microseconds ~ testcase, data=r.median, FUN=function(x) max(x) - min(x))

## ... and select the factors where the range is higher than the
## desired threshold ...
r.effect.threshold <- subset(r.range, microseconds > desired.delta)
r.effect.threshold
if (nrow(r.effect.threshold) == 0) {
    print("No runs differ in median by more than the desired effect")
    print("There is no need for more analysis, the results are not")
    print("meaningful, the statistics are not going to help")
    q(save="ask")
}

## ... analyze if the performance is significantly different by book
## type. First the mandatory plots ...
ggplot(data=data, aes(x=side, y=microseconds, color=booktype)) +
  geom_violin() +
  ylab("Elapsed Time (us)") +
  xlab("Side") +
  theme(legend.position="bottom")
ggplot(data=data, aes(x=side, y=microseconds, color=booktype)) +
  geom_boxplot() +
  ylab("Elapsed Time (us)") +
  xlab("Side") +
  theme(legend.position="bottom")

## ... calculate the median by book type ...
b.median <- aggregate(microseconds ~ booktype + side, data=data, FUN=median)
## ... print and visualize, in my experiments things are pretty
## obvious, but we want to use this as an opportunity to learn ...
summary(b.median)
ggplot(data=b.median, aes(x=booktype, y=microseconds, color=side)) +
  geom_point(size=4) +
  ylab("Elapsed Time (us)") +
  xlab("Book Type") +
  theme(legend.position="bottom")

## ... and then calculate the range ..
b.range <- aggregate(
    microseconds ~ side, data=b.median, FUN=function(x) max(x) - min(x))

## ... and select the factors where the range is higher than the
## desired threshold ...
b.effect.threshold <- subset(b.range, microseconds > desired.delta)
b.effect.threshold

## ... dumb users like me (coryan) need to be told what these results mean ...
if (nrow(b.effect.threshold) == 0) {
    print("No side differ in median by more than the desired effect")
    print("There is no need for more analysis, the results are not")
    print("meaningful, the statistics are not going to help")
    q(save="ask")
} else {
    print("Some of the sides have a estimated effect larger then the threshold")
    print("... we need to figure out if it was just luck")
}

w.median <- wilcox.test(microseconds ~ booktype, data)
print("Median latencies in microseconds by book type are:")
aggregate(microseconds ~ booktype, data=data, FUN=median)
if ((w.median$p.value < desired.significance)) {
    print(paste0("Using the Mann-Whitney U test, we reject the ",
                 "null hypothesis that both distributions are identical ",
                 "with significance ", desired.significance))
    n <- aggregate(microseconds ~ booktype, data=data, FUN=length)
    print(paste0("The Mann-Whitney U test p-value is ",
                 w.median$p.value,
                 ", counts: "))
    print(n)
}

q(save='no')
