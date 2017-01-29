FROM ubuntu:14.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update
RUN apt-get install -y software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update && apt-get install -y \
    libboost-atomic1.55.0 \
    libboost-chrono1.55.0 \
    libboost-context1.55.0 \
    libboost-date-time1.55.0 \
    libboost-filesystem1.55.0 \
    libboost-graph-parallel1.55.0 \
    libboost-graph1.55.0 \
    libboost-iostreams1.55.0 \
    libboost-locale1.55.0 \
    libboost-log1.55.0 \
    libboost-math1.55.0 \
    libboost-mpi-python1.55.0 \
    libboost-mpi1.55.0 \
    libboost-program-options1.55.0 \
    libboost-python1.55.0 \
    libboost-random1.55.0 \
    libboost-regex1.55.0 \
    libboost-serialization1.55.0 \
    libboost-signals1.55.0 \
    libboost-system1.55.0 \
    libboost-test1.55.0 \
    libboost-thread1.55.0 \
    libboost-timer1.55.0 \
    libboost-wave1.55.0 \
    libbz2-1.0 \
    libfftw3-3 \
    libyaml-cpp0.5 \
    ocl-icd-libopencl1 \
    zlib1g

RUN apt-get clean
