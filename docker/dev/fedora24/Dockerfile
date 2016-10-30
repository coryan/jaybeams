FROM fedora:24
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN dnf update -y && dnf install -y \
    autoconf \
    autoconf-archive \
    automake \
    boost \
    boost-devel \
    clang \
    compiler-rt \
    doxygen \
    fftw-devel \
    findutils \
    gcc-c++ \
    git \
    make \
    patch \
    tar \
    wget \
    yaml-cpp-devel
RUN dnf install -y \
    llvm \
    sudo

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3.2/skye-0.3.2.tar.gz
RUN tar -xf skye-0.3.2.tar.gz
RUN cd skye-0.3.2 && \
    CXX=g++ CC=gcc ./configure && \
    make check && \
    make install

WORKDIR /root
RUN /bin/rm -fr /var/tmp/build-skye

