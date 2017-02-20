#!/usr/bin/env Rscript

args <- commandArgs(TRUE)
if (length(args) < 3) {
    stop("Usage: bm_order_book_analyze.R <datafile> <figures> <reportname>")
}
data.filename <- args[1]
figures.path <- args[2]
report.name <- args[3]
script.root.dir <- getwd()
rm(args)

script.dirname <- function() {
    initial.options <- commandArgs(trailingOnly = FALSE)
    file.arg.name <- "--file="
    script.name <- sub(
        file.arg.name, "", initial.options[
                               grep(file.arg.name, initial.options)])
    return(dirname(script.name))
}

local({
    ## fall back on '/' if baseurl is not specified
    baseurl = servr:::jekyll_config('.', 'baseurl', '/')
    knitr::opts_knit$set(base.url = baseurl, root.dir = script.root.dir)
    ## fall back on 'kramdown' if markdown engine is not specified
    markdown = servr:::jekyll_config('.', 'markdown', 'kramdown')

    report.input <- paste(script.dirname(), 'bm_order_book_report.Rmd', sep='/')

    knitr::opts_chunk$set(fig.path = figures.path)
    knitr::opts_knit$set(width = 70, verbose = FALSE, progress = FALSE)

    ## see if we need to use the Jekyll render in knitr
    if (markdown == 'kramdown') {
        knitr::render_jekyll()
    } else {
        knitr::render_markdown()
    }
    if (!dir.exists(dirname(report.name))) {
        dir.create(dirname(report.name), recursive=TRUE)
    }
    knitr::knit(report.input, report.name, quiet = TRUE, encoding = 'UTF-8',
                envir = .GlobalEnv)
})
