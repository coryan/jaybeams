FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update && apt-get install -y \
      curl \
      libcairo-dev \
      libcurl4-openssl-dev \
      libssl-dev \
      r-base \
      r-base-dev \
      r-cran-boot \
      r-cran-ggplot2 \
      r-cran-plyr \
      r-cran-reshape2 \
      r-recommended && \
    apt-get clean

RUN echo 'install.packages(c("devtools"), \
    repos="http://cran.us.r-project.org", clean=TRUE)' > /tmp/packages.R && \
  Rscript /tmp/packages.R && rm /tmp/packages.R

RUN echo 'install.packages(c("DescTools", "knitr", "pwr", "svglite", "servr"), \
    repos="http://cran.us.r-project.org", clean=TRUE)' > /tmp/packages.R && \
  Rscript /tmp/packages.R && rm /tmp/packages.R

RUN echo "tryCatch({\n\
  require(DescTools)\n\
  require(boot)\n\
  require(ggplot2)\n\
  require(reshape2)\n\
  require(pwr)\n\
  require(parallel)\n\
  require(methods)\n\
  require(svglite)\n\
  require(devtools)\n\
  require(knitr)\n\
  require(servr)\n\
}, error = function(e) { print('ERROR'); print(e); q('no', 1) },\n\
   warning = function(w) { print('WARNING'); print(w); q('no', 1) }\n\
)\n\
q('no', 0)\n\
" >/tmp/test-packages.R && Rscript /tmp/test-packages.R

COPY bin/*.R /opt/jaybeams/bin/
COPY bin/*.Rmd /opt/jaybeams/bin/
