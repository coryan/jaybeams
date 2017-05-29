FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update && apt-get install -y \
      automake \
      clang \
      clang-format \
      cmake \
      doxygen \
      findutils \
      g++ \
      gcc \
      git \
      lcov \
      libboost-all-dev \
      libbz2-dev \
      libclang-dev \
      libclfft-dev \
      libfftw3-dev \
      libyaml-cpp-dev \
      llvm \
      lshw \
      make \
      ocl-icd-libopencl1 \
      ocl-icd-opencl-dev \
      opencl-headers \
      pkg-config \
      sudo \
      tar \
      time \
      wget \
      xz-utils \
      zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /var/tmp/install-autoconf-archive
RUN wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2016.03.20.tar.xz && \
    tar -xf autoconf-archive-2016.03.20.tar.xz && \
    cd autoconf-archive-2016.03.20 && ./configure --prefix=/usr && make && make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/install-autoconf-archive

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3.2/skye-0.3.2.tar.gz && \
    tar -xf skye-0.3.2.tar.gz && \
    cd skye-0.3.2 && \
    CXX=g++ CC=gcc CPPFLAGS=-D_GLIBCXX_USE_CXX11_ABI=1 ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu && \
    make check && \
    make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-skye

WORKDIR /var/tmp/build-pocl
RUN wget -q http://portablecl.org/downloads/pocl-0.13.tar.gz && \
    tar -zxf pocl-0.13.tar.gz && \
    cd pocl-0.13 && ./configure && make && make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-pocl

WORKDIR /var/tmp/build-boost-compute
RUN wget -q https://github.com/boostorg/compute/archive/boost-1.62.0.tar.gz && \
    tar -xf boost-1.62.0.tar.gz && \
    cd /var/tmp/build-boost-compute/compute-boost-1.62.0 && \
    cmake . && make && make DESTDIR=staging install && \
    cp -r staging/usr/local/include/compute/boost/compute.hpp /usr/include/boost/ && \
    cp -r staging/usr/local/include/compute/boost/compute/ /usr/include/boost/ && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-boost-compute

WORKDIR /root
