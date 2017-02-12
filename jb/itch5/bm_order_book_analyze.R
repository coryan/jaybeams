#!/usr/bin/env Rscript
require(ggplot2)
require(boot)
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

print("Examine data.plot.*, observe if there are any obvious correlations in the data")
## ... but really it is not, lots of auto-correlation ...
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

print(paste0("max.acf.array=", round(max.acf.array, 4)))
print(paste0("max.acf.map=", round(max.acf.map, 4)))

## ... we set the maximum allowed auto-correlation to 5% ...
max.autocorrelation <- 0.05

if (max.acf.array >= max.autocorrelation) {
	stop("Some evidence of auto-correlation in array data max-acf=", round(max.acf.array, 4))
}
if (max.acf.map >= max.autocorrelation) {
	stop("Some evidence of auto-correlation in array data max-acf=", round(max.acf.map, 4))
}

##
## Power analysis, validate the data had enough samples
###
print("Estimating standard deviation on the data to validate power:")

## ... use bootstraping to estimate the standard deviation ...                                    
sd.estimator <- function(D,i) {
    b=D[i,];
    return(sd(b$microseconds));
}

b.array <- boot(data=subset(data, book_type == 'array'), R=10000, statistic=sd.estimator)
b.map <- boot(data=subset(data, book_type == 'map'), R=10000, statistic=sd.estimator)
plot(b.array)
plot(b.map)

ci.array <- boot.ci(b.array, type=c('perc', 'norm', 'basic'))
ci.map <- boot.ci(b.map, type=c('perc', 'norm', 'basic'))

estimated.sd <- ceiling(max(ci.map$percent[[4]], ci.array$percent[[4]],
            ci.map$basic[[4]], ci.array$basic[[4]],
            ci.map$normal[[3]], ci.array$normal[[3]]))

print(paste0("Estimated standard deviation: ", round(estimated.sd, 2)))
            
test.iterations <- 20000
clock.ghz <- 3
## ... this is the minimum effect size that we
## are interested in, anything larger is great,
## smaller is too small to care ...
min.delta <- 1.0 / (clock.ghz * 1000.0) * test.iterations
print(paste0("Minimum desired effect: ", min.delta))

min.samples <- 5000
desired.delta <- min.delta
desired.significance <- 0.01
desired.power <- 0.95
nonparametric.extra.cost <- 1.15

## ... the power object has several
## interesting bits, so store it ...
desired.pwr.object <- power.t.test(
    delta=desired.delta, sd=estimated.sd,
    sig.level=desired.significance, power=desired.power)

## ... I like multiples of 1000 because
## they are easier to type and say ...
required.nsamples <-
    1000 * ceiling(nonparametric.extra.cost *
                   desired.pwr.object$n / 1000)
required.nsamples

if (required.nsamples > length(data.array.ts)) {
	stop("Not enough samples in 'array' data, required=", required.nsamples,
	     " actual=", length(data.array.ts))
}

if (required.nsamples > length(data.map.ts)) {
	stop("Not enough samples in 'map' data, required=", required.nsamples,
	     " actual=", length(data.map.ts))
}

##
## Assuming all the requirements pass, run the statistical test:
##
data.mw <- wilcox.test(microseconds ~ book_type, data=data, conf.int=TRUE)
estimated.delta <- data.mw$estimate

if (abs(estimated.delta) < desired.delta) {
	stop("The estimated effect is too small to draw any conclusions.  Estimated=", estimated.delta,
	     " desired.delta=", desired.delta)
}

if (data.mw$p.value >= desired.significance) {
	print(paste0("The p-value (", round(data.mw$p.value, 4), ") is higher than the desired",
	             " significance level (", desired.significance, ").  The null hypothesis that both",
	             " the 'array' and 'map' based order books have the same performance cannot be rejected.",
	             " Therefore we treat both as having the same performance."))
} else {
	print(paste0("The p-value (", round(data.mw$p.value, 4), ") is below the desired",
	             " significance level (", desired.significance, ").  The null hypothesis that ",
	             " both the 'array' and 'map' book have the same performance is rejected.",
	             " Therefore we assume that their performance is different.  The ",
	             " Hodges-Lehmann estimator values this difference in performance at ",
	             round(data.mw$estimate, 2), " microseconds"))
}

q(save='no')

