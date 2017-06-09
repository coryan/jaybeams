FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

# ... update the apt repository list and install the specific version
# of the Ubuntu packages we need ...
RUN apt-get update && apt-get install -y --no-install-recommends \
      curl=7.47.0-1ubuntu2.2 \
      libcairo2-dev=1.14.6-1 \
      libcurl4-openssl-dev=7.47.0-1ubuntu2.2 \
      libssl-dev=1.0.2g-1ubuntu4.8 \
      r-base=3.2.3-4 \
      r-base-dev=3.2.3-4 \
      r-cran-boot=1.3-17-1 \
      r-cran-ggplot2=2.0.0-2 \
      r-cran-plyr=1.8.3-1 \
      r-cran-reshape2=1.4-1 \
      r-recommended=3.2.3-4 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# ... install the devtool R package to install specific version of the
# other R packages we need ...
RUN echo 'install.packages(c("devtools"), \
    repos="http://cran.us.r-project.org", clean=TRUE)' > /tmp/packages.R && \
  Rscript /tmp/packages.R && rm /tmp/packages.R

# ... install the other R packages we need ...
RUN printf 'options(repos = "http://cran.us.r-project.org")\n\
  devtools::install_version("DescTools", version="0.99.9")\n\
  devtools::install_version("knitr", version="1.16")\n\
  devtools::install_version("pwr", version="1.2-1")\n\
  devtools::install_version("svglite", version="1.2.0")\n\
  devtools::install_version("servr", version="0.6")\n' > /tmp/packages.R && \
  Rscript /tmp/packages.R && rm /tmp/packages.R

# ... verify we have successfully installed the necessary packages ...
RUN printf "tryCatch({\n\
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
   warning = function(w) { print('WARNING'); print(w); q('no', 0) }\n\
)\n\
q('no', 0)\n" >/tmp/test-packages.R && Rscript /tmp/test-packages.R

# ... copy the JayBeam scripts into the docker image ...
COPY bin/*.R /opt/jaybeams/bin/
COPY bin/*.Rmd /opt/jaybeams/bin/
