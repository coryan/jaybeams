#!/usr/bin/env Rscript

## Run the program and capture its output ...
rd.out = as.double(readLines(
    pipe("jb/testing/check_mt19937_initializer --iterations=10000")))

## ... print some summary data for debugging ..
summary(rd.out)

## ... also some ugly graphs ...
plot(ts(rd.out))

## ... compute the auto-correlation ...
rd.acf <- acf(ts(rd.out))

## ... except for lag == 1 (which is always 1.0) it must be smaller
## than 0.05 ...
rd.max <- max(abs(rd.acf$acf[seq(2,length(rd.acf$acf))]))
print(rd.max)
stopifnot(rd.max < 0.05)

## ... just exit at the end ...
q(save="no")
