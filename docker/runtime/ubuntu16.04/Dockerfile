FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update && apt-get install -y \
    binutils \
    coreutils \
    cpufrequtils \
    debianutils \
    libboost-atomic1.58.0 \
    libboost-chrono1.58.0 \
    libboost-context1.58.0 \
    libboost-coroutine1.58.0 \
    libboost-date-time1.58.0 \
    libboost-filesystem1.58.0 \
    libboost-graph-parallel1.58.0 \
    libboost-graph1.58.0 \
    libboost-iostreams1.58.0 \
    libboost-locale1.58.0 \
    libboost-log1.58.0 \
    libboost-math1.58.0 \
    libboost-mpi-python1.58.0 \
    libboost-mpi1.58.0 \
    libboost-program-options1.58.0 \
    libboost-python1.58.0 \
    libboost-random1.58.0 \
    libboost-regex1.58.0 \
    libboost-serialization1.58.0 \
    libboost-signals1.58.0 \
    libboost-system1.58.0 \
    libboost-test1.58.0 \
    libboost-thread1.58.0 \
    libboost-timer1.58.0 \
    libboost-wave1.58.0 \
    libbz2-1.0 \
    libclfft2 \
    libfftw3-3 \
    libyaml-cpp0.5v5 \
    lshw \
    mawk \
    procps \
    ocl-icd-libopencl1 \
    sudo \
    time \
    zlib1g && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/*

COPY bin /opt/jaybeams/bin/
COPY lib /opt/jaybeams/lib/
COPY include /opt/jaybeams/include/
