#!/usr/bin/env Rscript
require(boot)
require(ggplot2)
require(parallel)
require(pwr)

## ... make the graphs look pretty ...
golden.ratio <- (1+sqrt(5))/2
svg.w <- 8.0
svg.h <- svg.w / golden.ratio
png.w <- 960
png.h <- round(png.w / golden.ratio)

## Get the command line arguments ...
args <- commandArgs(trailingOnly=TRUE)
data.filename <- 'bm_order_book_generate.1.results.csv'
if (length(args) > 0) {
   data.filename <- args[1]
}

data <- read.csv(
    file=data.filename, header=FALSE, comment.char='#',
    col.names=c('book_type', 'nanoseconds'))

## ... I prefer microseconds because they are easier to think about ...
data$microseconds <- data$nanoseconds / 1000.0
data$idx <- ave(data$microseconds, data$book_type, FUN=seq_along)

# ... print the summary, to give the user a sense of where things are ...
print("Summary of data:")
print(summary(data))

print("Summary by order book type:")
print(aggregate(microseconds ~ book_type, data=data, FUN=summary))

# ... just visualize the data as a density function ...
ggplot(data=data, aes(x=microseconds, color=book_type)) +
    theme(legend.position="bottom") + facet_grid(book_type ~ .) + stat_density()

ggplot(data=data, aes(y=microseconds, x=book_type, color=book_type)) +
    theme(legend.position="bottom") + geom_boxplot()

##
## Validate the data is independent
##

# ... check that the data looks independent ...
ggplot(data=data, aes(x=idx, y=microseconds, color=book_type)) +
    theme(legend.position="bottom") + facet_grid(book_type ~ .) + geom_point()
ggsave(width=svg.w, heigh=svg.h, filename='data.plot.svg')
ggsave(width=svg.w, heigh=svg.h, filename='data.plot.pdf')
ggsave(width=svg.w, heigh=svg.h, filename='data.plot.png')

print(paste0(
    "Examine the data.plot.* files, observe if there are any",
    " obvious correlations in the data"))
print("")

## ... but really it is not, lots of auto-correlation ...
print("Checking for independence of samples...")
print("  (or more precisely lack of auto-correlation in the samples)")
data.array.ts <- ts(subset(data, book_type == 'array')$microseconds)
data.map.ts <- ts(subset(data, book_type == 'map')$microseconds)

par(mfrow=c(2,1))
acf(data.array.ts)
acf(data.map.ts)

svg(width=svg.w, height=svg.h, filename='data.acf.svg')
par(mfrow=c(2,1))
acf(data.array.ts)
acf(data.map.ts)
dev.off()

png(width=png.w, height=png.h, filename='data.acf.png')
par(mfrow=c(2,1))
acf(data.array.ts)
acf(data.map.ts)
dev.off()

max.acf.array <- max(abs(tail(acf(data.array.ts, plot=FALSE)$acf, -1)))
max.acf.map <- max(abs(tail(acf(data.map.ts, plot=FALSE)$acf, -1)))

## ... we set the maximum allowed auto-correlation to 5% ...
max.autocorrelation <- 0.05

print(paste0("  we will reject data with auto-correlation factors >= ",
             max.autocorrelation))
print(paste0("  array max.acf= ", round(max.acf.array, 4)))
print(paste0("  map   max.acf= ", round(max.acf.map, 4)))

if (max.acf.array >= max.autocorrelation) {
    stop("Some evidence of auto-correlation in array data max-acf=",
         round(max.acf.array, 4))
}
if (max.acf.map >= max.autocorrelation) {
    stop("Some evidence of auto-correlation in array data max-acf=",
         round(max.acf.map, 4))
}

print("PASSED: the samples do not exhibit excessive auto-correlation")

##
## Power analysis, validate the data had enough samples
##

## ... use bootstraping to estimate the standard deviation ...
sd.estimator <- function(D,i) {
    return(sd(D[i,'microseconds']));
}

print("Estimating standard deviation on the 'array' data ...")
core.count <- detectCores()
b.array <- boot(data=subset(data, book_type == 'array'), R=10000,
                statistic=sd.estimator, parallel="multicore", ncpus=core.count)
ci.array <- boot.ci(b.array, type=c('perc', 'norm', 'basic'))
print(ci.array)

print("Estimating standard deviation on the 'map' data ...")
b.map <- boot(data=subset(data, book_type == 'map'), R=10000,
              statistic=sd.estimator, parallel="multicore", ncpus=core.count)
ci.map <- boot.ci(b.map, type=c('perc', 'norm', 'basic'))
print(ci.map)

png(width=png.w, height=png.h, filename='boot.array.sd.png')
plot(b.array)
dev.off()
png(width=png.w, height=png.h, filename='boot.map.sd.png')
plot(b.map)
dev.off()

svg(width=svg.w, height=svg.h, filename='boot.array.sd.svg')
plot(b.array)
dev.off()
svg(width=svg.w, height=svg.h, filename='boot.map.sd.svg')
plot(b.map)
dev.off()

estimated.sd <- ceiling(max(ci.map$percent[[4]], ci.array$percent[[4]],
            ci.map$basic[[4]], ci.array$basic[[4]],
            ci.map$normal[[3]], ci.array$normal[[3]]))

print(paste0("Estimated standard deviation: ", round(estimated.sd, 2)))

##
## Perform power analysis
##
print("Performing power analysis to determine if sample size is large enough")

test.iterations <- 20000
clock.ghz <- 3
## ... this is the minimum effect size that we
## are interested in, anything larger is great,
## smaller is too small to care ...
min.delta <- 1.0 / (clock.ghz * 1000.0) * test.iterations

min.samples <- 5000
desired.delta <- min.delta
desired.significance <- 0.01
desired.power <- 0.95
nonparametric.extra.cost <- 1.15

## ... if we do not have enough power to detect 10 times the minimum
## effect we abort the program ...
required.pwr.object <- power.t.test(
    delta=10 * desired.delta, sd=estimated.sd,
    sig.level=desired.significance, power=desired.power)

## ... I like multiples of 1000 because they are easier to type and say ...
required.nsamples <-
    1000 * ceiling(nonparametric.extra.cost *
                   required.pwr.object$n / 1000)

print(paste0(
    "Required samples to detect an effect of ", round(10 * desired.delta, 2),
    " microseconds is ", required.nsamples))

if (required.nsamples > length(data.array.ts)) {
    stop("Not enough samples in 'array' data to detect expected effect (",
         10 * desired.delta, ") should be >=", required.nsamples,
         " actual=", length(array.map.ts))
}

if (required.nsamples > length(data.map.ts)) {
    stop("Not enough samples in 'map' data to detect expected effect (",
         10 * desired.delta, ") should be >=", required.nsamples,
         " actual=", length(map.map.ts))
}

print("PASSED: The samples have the minimum required power")

## ... give out a warning if the power level is not enough to detect
## minimum effects, though we expect much higher than the minimum ...
desired.pwr.object <- power.t.test(
    delta=10 * desired.delta, sd=estimated.sd,
    sig.level=desired.significance, power=desired.power)
desired.nsamples <-
    1000 * ceiling(nonparametric.extra.cost *
                   desired.pwr.object$n / 1000)

print(paste0(
    "Required samples to detect an effect of ", round(desired.delta, 2),
    " microseconds is ", desired.nsamples))

if (desired.nsamples > length(data.array.ts)) {
    warning("Not enough samples in 'array' data to detect minimum effect (",
            desired.delta, ") should be >=", desired.nsamples,
            " actual=", length(array.map.ts))
}

if (required.nsamples > length(data.map.ts)) {
    warning("Not enough samples in 'map' data to detect minimum effect (",
            desired.delta, ") should be >=", desired.nsamples,
            " actual=", length(data.map.ts))
}

##
## Assuming all the requirements pass, run the statistical test:
##
data.mw <- wilcox.test(microseconds ~ book_type, data=data, conf.int=TRUE)
estimated.delta <- data.mw$estimate

if (abs(estimated.delta) < desired.delta) {
    stop("The estimated effect is too small to draw any conclusions.",
         " Estimated effect=", estimated.delta,
         " minimum effect=", desired.delta)
}

print(paste0(
    "PASSED: the estimated effect (", round(estimated.delta, 2),
    ") is large enough to be considered"))

print(paste0(
    "We use the Mann-Whitney U test to determine if the results",
    " are statistically significant at the alpha=",
    round(desired.significance, 2), " level."))

if (data.mw$p.value >= desired.significance) {
    print(paste0(
        "The test p-value (", round(data.mw$p.value, 4),
        ") is larger than the desired significance level of",
        " alpha=", round(desired.significance, 4), "."))
    print(paste0(
        "Therefore we CANNOT REJECT the null hypothesis that",
        " both the 'array' and 'map' based",
        " order books have the same performance."))
} else {
    interval <- paste0(round(data.mw$conf.int, 2), collapse=',')
    print(paste0(
        "The test p-value (", round(data.mw$p.value, 4),
        ") is smaller than the desired significance level of",
        " alpha=", round(desired.significance, 4), "."))
    print(paste0(
        "Therefore we REJECT the null hypothesis that",
        " both the 'array' and 'map' based",
        " order books have the same performance."))
    print(paste0(
        "The effect is quantified using the Hodges-Lehmann estimator,",
        " which is compatible with the Mann-Whitney U test, the estimator",
        " value is ", round(data.mw$estimate, 2), " microseconds with a",
        " 95% confidence interval of [", interval, "]"))
}

q(save='no')

