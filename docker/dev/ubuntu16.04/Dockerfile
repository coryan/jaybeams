FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

#'
RUN apt-get update && apt-get install -y \
      autoconf \
      automake \
      build-essential \
      ca-certificates \
      clang \
      clang-format \
      cmake \
      doxygen \
      findutils \
      g++ \
      gcc \
      git \
      golang \
      lcov \
      libboost-all-dev \
      libbz2-dev \
      libclang-dev \
      libclfft-dev \
      libfftw3-dev \
      libtool \
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
      unzip \
      wget \
      xz-utils \
      zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /var/tmp/install-autoconf-archive
RUN wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2016.03.20.tar.xz && \
    tar -xf autoconf-archive-2016.03.20.tar.xz && \
    (cd autoconf-archive-2016.03.20 && \
      ./configure --prefix=/usr && make && make install) && \
    /bin/rm -fr /var/tmp/install-autoconf-archive

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3.2/skye-0.3.2.tar.gz && \
    tar -xf skye-0.3.2.tar.gz && \
    (cd skye-0.3.2 && \
      CXX=g++ CC=gcc CPPFLAGS=-D_GLIBCXX_USE_CXX11_ABI=1 ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu && \
      make check && \
      make install) && \
    /bin/rm -fr /var/tmp/build-skye

WORKDIR /var/tmp/build-pocl
RUN wget -q http://portablecl.org/downloads/pocl-0.13.tar.gz && \
    tar -zxf pocl-0.13.tar.gz && \
    (cd pocl-0.13 && ./configure && make && make install) && \
    /bin/rm -fr /var/tmp/build-pocl

WORKDIR /var/tmp/build-boost-compute
RUN wget -q https://github.com/boostorg/compute/archive/boost-1.62.0.tar.gz && \
    tar -xf boost-1.62.0.tar.gz && \
    (cd compute-boost-1.62.0 && \
      cmake . && make && make DESTDIR=staging install && \
      cp -r staging/usr/local/include/compute/boost/compute.hpp \
          /usr/include/boost/ && \
      cp -r staging/usr/local/include/compute/boost/compute/ \
          /usr/include/boost/) && \
    /bin/rm -fr /var/tmp/build-boost-compute

WORKDIR /var/tmp/build-beast-http
RUN wget -q https://github.com/vinniefalco/Beast/archive/v1.0.0-b30.tar.gz && \
    tar -xf v1.0.0-b30.tar.gz && \
    (cd /var/tmp/build-beast-http/Beast-1.0.0-b30/ && \
      cmake . && make -j 2 && cp -r include/beast /usr/include/beast/) && \
    /bin/rm -fr /var/tmp/build-beast-http

WORKDIR /var/tmp/build-grpc
RUN git clone --depth 10 --branch v1.3.4 https://github.com/grpc/grpc.git && \
  (cd /var/tmp/build-grpc/grpc/ && \
    git submodule update --init && \
    make -j 2 && make install && \
    cd /var/tmp/build-grpc/grpc/third_party/protobuf && \
    make install) && \
  /bin/rm -fr /var/tmp/build-grpc

WORKDIR /var/tmp/install-etcd
RUN wget -q https://github.com/coreos/etcd/releases/download/v3.2.1/etcd-v3.2.1-linux-amd64.tar.gz && \
  tar -xf etcd-v3.2.1-linux-amd64.tar.gz && \
  cp etcd-v3.2.1-linux-amd64/etcd /usr/bin && \
  cp etcd-v3.2.1-linux-amd64/etcdctl /usr/bin && \
  /bin/rm -fr /var/tmp/install-etcd

# TODO() this should be installed in that first "apt-get install", need to
# debug the image first.
RUN apt-get update && apt-get install -y google-mock && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /root

