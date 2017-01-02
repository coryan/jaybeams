#!/usr/bin/env Rscript
require(ggplot2)
require(svglite)
require(boot)
require(pwr)
require(DescTools)
require(MASS)

## How much data do we need to collect to make the analysis powerful
## enough?  We need to do some exploratory data analysis to answer
## just that question.  What we do is collect some data, obtain some
## statistics about it and then *discard it*.

desired.significance <- 0.01

args <- commandArgs(trailingOnly=TRUE)
explore.file <- args[1]

data <- read.csv(
    explore.file, header=FALSE, col.names=c('testcase', 'nanoseconds'),
    comment.char='#')
data$run <- factor('explore')
## express the time in microseconds
data$microseconds <- data$nanoseconds / 1000.0

## extract the booktype and side from the combined testcase column
data$booktype <- factor(sapply(
    data$testcase,
    function(x) strsplit(as.character(x), split=':')[[1]][1]))
data$side <- factor(sapply(
    data$testcase,
    function(x) strsplit(as.character(x), split=':')[[1]][2]))

## What is the median, mean, sd of the data by testcase?
data.mean <- aggregate(microseconds ~ testcase, data=data, FUN=mean)
data.sd <- aggregate(microseconds ~ testcase, data=data, FUN=sd)
data.median <- aggregate(microseconds ~ testcase, data=data, FUN=median)
data.summary <- aggregate(microseconds ~ testcase, data=data, FUN=summary)

## We are going to need non-parametric statistics, that is a lot of
## extra work, and we should convince ourselves that it is worth it.
## For starters let's show that normal distributions will not work:

## Generate some ugly graphs, and test for normality ..
op <- par(mfrow=c(1, length(levels(data$testcase))))
for(test in levels(data$testcase)) {
    d <- subset(data, testcase == test)
    qqnorm(d$microseconds, xlab=as.character(test), main=test)
    qqline(d$microseconds)
    ## the shapiro.test function cannot handle more than 5,000 samples, so
    ## we resample the data, I know this is ugly ...
    s <- shapiro.test(sample(d$microseconds, 5000))
    if (s$p.value < desired.significance) {
        print(paste0(
            "Normality for ", test, " is REJECTED at ",
            desired.significance, " significance.  We used ",
            s$method, " with N=", nrow(d),
            " (p-value=", s$p.value, ", W=", s$statistic, ")"))
    } else {
        print(paste0(
            "Normality for ", test, " cannot be rejected at ",
            desired.significance, " significance.  We used ",
            s$method, " with N=", nrow(d),
            " (p-value=", s$p.value, ", W=", s$statistic, ")"))
    }
    rm(d, s)
}
par(op)
rm(test, op)
## ... a prettier plot, but without the nice line.  If the data was
## normal the points should be in a straight line ...
ggplot(data) +
    facet_grid(booktype ~ side, scales="free") +
    stat_qq(aes(sample=microseconds, color=testcase))
ggsave(filename="bm_order_book.test.normality.svg",
       width=8.0, height=8.0/1.61)
ggsave(filename="bm_order_book.test.normality.png",
       width=8.0, height=8.0/1.61)

## ... Okay, we give up on the normal distribution, what about
## exponential? Some more ugly graphs convince me that it is not worth
## pursuing ...
op <- par(mfrow=c(1, length(levels(data$testcase))))
for(test in levels(data$testcase)) {
    d <- subset(data, testcase == test & microseconds > 0)
    p <- ppoints(100)
    q <- quantile(d$microseconds, p=p)
    plot(qexp(p), q, xlab=as.character(test), main=test)
    qqline(d$microseconds)
    rm(d, p, q)
}
par(op)
rm(test, op)
## ... a prettier plot, but without the nice line.  If the data
## distributed exponential the points should be in a straight line
## ...
ggplot(data) +
    facet_grid(booktype ~ side, scales="free") +
    stat_qq(aes(sample=microseconds, color=testcase), distribution=stats::qexp)
ggsave(filename="bm_order_book.test.exponential.svg",
       width=8.0, height=8.0/1.61)
ggsave(filename="bm_order_book.test.exponential.png",
       width=8.0, height=8.0/1.61)

## ... I give up, though we should run the tests with all common
## distributions, probably using ks.test() to reject each one, and so
## forth ...
##          Distributions ‘"beta"’, ‘"cauchy"’, ‘"chi-squared"’,
##          ‘"exponential"’, ‘"f"’, ‘"gamma"’, ‘"geometric"’,
##          ‘"log-normal"’, ‘"lognormal"’, ‘"logistic"’, ‘"negative
##          binomial"’, ‘"normal"’, ‘"Poisson"’, ‘"t"’ and ‘"weibull"’
##          are recognised, case being ignored.

## ... Assuming we have rejected all common distributions we are
## doomed to use non-parametric statistics.  We still need to do some
## power analysis.  The good news is that the references I have found
## [1] suggest that only 15% additional data make the non-parametric
## tests (or at least the Mann-Whitney U test), as powerful as the
## regular t-test. That is trivial for benchmarks that run in a few
## minutes or even a couple of hours ...
##
## [1]: http://www.jerrydallal.com/lhsp/npar.htm
##  https://www.graphpad.com/guides/prism/6/statistics/index.htm?stat_sample_size_for_nonparametric_.htm
##
non.parametric.overhead <- 1.15


## Statistical fanciness aside, the important thing is to think about
## the problem.  We are optimizing book building code, we want to say
## that one version is faster than other version.  In this benchmark
## each iteration of the test represents 100,000 operations, because
## one operation is too fast (a few nanoseconds) to be even measured:
iteration.operations <- 100000

## We want each iteration to be at least 1 cycle faster, when this is
## being written (and for the foreseeable future) computers run at
## around 3.0Ghz, so a cycle is about 1/3 nanoseconds, all the
## measurements are in microseconds, so that becomes ...
desired.improvement.ns <- 1.0 / 3.0
desired.improvement <- desired.improvement.ns / 1000.0

## ... so the desired delta for each iteration is:
desired.delta <- desired.improvement * iteration.operations

## ... in previous runs we have observed around this much standard deviation:
observed.sd <- max(data.sd$microseconds)
observed.sd <- ceiling(observed.sd / 100) * 100

## ... if we use the conventional values for power analysis ...
desired.power <- 0.8
desired.significance <- 0.05
p <- power.t.test(n=NULL, sig.level=desired.significance, power=desired.power,
                  delta=desired.delta, sd=observed.sd)
## print(p)

## ... if we use more strict values ...
desired.power <- 0.95
desired.significance <- 0.01
p <- power.t.test(n=NULL, sig.level=desired.significance, power=desired.power,
                  delta=desired.delta, sd=observed.sd)
## print(p)

## 5000 samples is so easy to collect that we should get them anyway,
## even if the test does not need that many ...
min.samples <- max(5000, non.parametric.overhead * p$n)

## ... round up to the nearest multiple of 1000 ...
min.samples <- ceiling(min.samples / 1000) * 1000

print(paste0("The tests should be run with at least ", min.samples, " samples"))
print(paste0("  review the numbers if the standard deviation is higher than ",
             observed.sd))
print(paste0("  or if you are interested in results at higher than p=",
             desired.significance, " significance"))

q(save='no')
