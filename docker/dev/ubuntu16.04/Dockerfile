FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update && apt-get install -y \
    autoconf-archive \
    automake \
    g++ \
    gcc \
    git \
    libboost-all-dev \
    libfftw3-dev \
    libyaml-cpp-dev \
    make \
    wget

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3/skye-0.3.tar.gz
RUN tar -xf skye-0.3.tar.gz
RUN cd skye-0.3 && \
    CXX=g++ CC=gcc CPPFLAGS=-D_GLIBCXX_USE_CXX11_ABI=1 ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu && \
    make check && \
    make install

