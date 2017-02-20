#!/usr/bin/env Rscript

args <- commandArgs(TRUE)
if (length(args) < 3) {
    stop("Usage: bm_order_book_analyze.R <datafile> <figures> <reportname>")
}
data.filename <- args[1]
figures.path <- args[2]
report.name <- args[3]
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
    knitr::opts_knit$set(base.url = baseurl)
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
    report.md <- paste0(report.name, '.md')
    knitr::knit(report.input, report.md, quiet = TRUE, encoding = 'UTF-8',
                envir = .GlobalEnv)
#    name.html <- paste0(report.name, '.html')
#    knitr::render_html()
#    knitr::knit(report.input, report.html, quiet = TRUE, encoding = 'UTF-8',
#                envir = .GlobalEnv)
})
