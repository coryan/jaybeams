#!/usr/bin/env Rscript
require(ggplot2)
require(boot)
require(pwr)

## How much data do we need to collect to make the test powerful enough?

## We want each iteration to be at least 5 nanoseconds faster, all the
## measurements are in microseconds, so that becomes ...
desired.improvement <- 5 / 1000.0
## ... we run this many operations per iteration:
iteration.operations <- 100000
## ... so the desired delta for each iteration is:
desired.delta <- desired.improvement * iteration.operations
## ... in previous runs we have observed around this much standard deviation:
observed.sd <- 600
## ... and it may not matter but we have observed about this for the mean
observed.mean <- 6100

## ... if we use the conventional values for power analysis ...
power.t.test(n=NULL, sig.level=0.05, power=0.8,
             delta=desired.delta, sd=observed.sd)

## ... if we use more strict values ...
p <- power.t.test(n=NULL, sig.level=0.01, power=0.95,
             delta=desired.delta, sd=observed.sd)
p

## "it has been shown" [1] that we only need 15% more data to apply the
## Mann-Whitney U test.  I have not been able to find a reference, but two
## sources say so.  In any case, the number produce above is trivial
## (< 100 samples), generating 5,000 costs nothing in a computer.
##
## [1]: http://www.jerrydallal.com/lhsp/npar.htm
##  https://www.graphpad.com/guides/prism/6/statistics/index.htm?stat_sample_size_for_nonparametric_.htm

min.samples <- max(5000, 1.15 * p$n)

## Pick your code branch here ...
use.args <- TRUE
use.hardcoded <- FALSE
use.download <- FALSE

if (use.args) {
    args <- commandArgs(trailingOnly=TRUE)

    baseline.file <- args[1]
    candidate.file <- args[2]
}
if (use.hardcoded) {
    baseline.file <- '/home/coryan/jaybeams/gcc/bm_order_book_results.14622.csv'
    candidate.file <- '/home/coryan/jaybeams/gcc/bm_order_book_results.14819.csv'
}
if (use.args | use.hardcoded) {
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
}

if (use.download) {
    prefix <- '/home/coryan/Downloads/build-02/bm_order_book_results'
    data <- data.frame()
    for (run in c('4703', '4737', '4780', '4808', '4836')) {
        file <- paste(prefix, run, 'csv', sep='.')
        d <- read.csv(
            file, header=FALSE, col.names=c('testcase', 'nanoseconds'),
            comment.char='#')
        d$run <- factor(run)
        data <- rbind(data, d)
    }
}


## express the time in microseconds
data$microseconds <- data$nanoseconds / 1000.0

## extract the booktype and side from the combined testcase column
data$booktype <- factor(sapply(
    data$testcase,
    function(x) strsplit(as.character(x), split=':')[[1]][1]))
data$side <- factor(sapply(
    data$testcase,
    function(x) strsplit(as.character(x), split=':')[[1]][2]))

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

r.range <- aggregate(microseconds ~ testcase, data=aggregate(microseconds ~ run + testcase, data=data, FUN=median), FUN=function(x) max(x) - min(x))

r.effect.threshold <- subset(r.range, microseconds > desired.delta)
r.effect.threshold
if (nrow(r.effect.threshold) == 0) {
    print("No runs differ in median by more than the desired effect")
    print("There is no need for more analysis, the results are not")
    print("meaningful, the statistics are not going to help")
    q(save="ask")
}


d <- subset(data, testcase=='array:sell')
p <- pairwise.wilcox.test(x=d$microseconds, g=d$run)

wilcox.test(x=subset(d, run=='4808')$microseconds, y=subset(d, run=='4780')$microseconds)

runs <- levels(data$run)
for (candidate in tail(runs, -1)) {
    baseline <- runs[[1]]
    for (testcase in levels(data$testcase)) {
        wilcox.test(
    }
}



## ... is the 
w.array.buy <- wilcox.test(
    microseconds ~ run, data=subset(data, testcase=='array:buy'))

aggregate(microseconds ~ run, FUN=sd, data=subset(data, testcase=='array:buy'))


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
