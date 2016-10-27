FROM fedora:23
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN dnf update -y && dnf install -y \
    autoconf \
    autoconf-archive \
    automake \
    boost \
    boost-devel \
    clang \
    doxygen \
    fftw-devel \
    gcc-c++ \
    git \
    make \
    patch \
    wget \
    yaml-cpp-devel
RUN dnf install -y tar

WORKDIR /var/tmp
RUN git clone --depth 2 https://github.com/coryan/jaybeams.git
WORKDIR /var/tmp/jaybeams
RUN patch -d / -p0 < m4/ax_cxx_compile_stdcxx.m4.patch

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3/skye-0.3.tar.gz
RUN tar -xf skye-0.3.tar.gz
RUN cd skye-0.3 && \
    CXX=g++ CC=gcc ./configure && \
    make check && \
    make install

