#!/usr/bin/env Rscript
require(ggplot2)

## How much data do we need to collect to obtain statiscally
## significant results?  These values ared documented in bm_order_book_analyze_initial.R:
min.samples <- 5000
expected.sd <- 200
desired.delta <- (1.0/3.0)/1000.0 * 100000
desired.significance <- 0.01
desired.power <- 0.95

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
    print(paste0("The test had enough samples"));
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
print("Median latency by run and testcase")
print(r.median)

ggplot(data=r.median, aes(x=testcase, y=microseconds, color=run)) +
  geom_point(size=4) +
  ylab("Elapsed Time (us)") +
  xlab("Test Case") +
  theme(legend.position="bottom")

## ... we want to know if the performance changed for the 'array'
## booktype (the map booktype is not changing), so select that first
## ...
data.array <- subset(data, booktype == 'array')

## ... now we want to see if there is a meaningful, and statistically
## significant difference for the performance between both runs ...
print("Median latencies in microseconds by run are:")
print(aggregate(microseconds ~ run, data=data.array, FUN=median))

## ... calculate the range across all runs ...
r.range <- aggregate(
    microseconds ~ testcase, data=r.median, FUN=function(x) max(x) - min(x))

## ... this is the statistically test we are using ...
w.run <- wilcox.test(microseconds ~ run, data.array, conf.int=TRUE)
if (abs(w.run$estimate) < desired.delta) {
    print(paste0("We cannot reject the null hypothesis,",
                  " the candidate and baseline data",
		  " location parameters difference is smaller",
		  " than the desired effect: ", desired.delta))
} else if (w.run$p.value >= desired.significance) {
    print(paste0("We cannot reject the null hypothesis,",
                  " the Mann-Whitnet U test p-value is larger",
		  " than the desired significance level of",
		  " alpha=", desired.significance))
} else {
    print(paste0("Using the Mann-Whitney U test, we reject the",
                 " null hypothesis that both runs (candidate vs. baseline)",
		 " are identical, at the significance level of",
		 " alpha=", desired.significance,
		 ".  Furthermore, the effect of the changes in",
		 " the candidate is quantified using the",
		 " Hodges–Lehmann (HL) estimator as: ",
		 w.run$estimate))
}
print(w.run)
cat("[press [enter] to continue]")
readLines("stdin", n=1)

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
} else {
    print(paste0("We cannot reject the null hypothesis, ",
                  "the map and array books are not ",
		  "statistically different at the alpha=",
		  desired.significance, " level"))
     print(w.median)
}
cat("[press [enter] to continue]")
readLines("stdin", n=1)

q(save='no')
