FROM ubuntu:14.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && apt-get install -y \
      automake \
      clang-3.8 \
      clinfo \
      cmake \
      doxygen \
      g++-5 \
      gcc-5 \
      git \
      lshw \
      lcov \
      libboost1.55-all-dev \
      libbz2-dev \
      libclang-3.8-dev \
      libfftw3-dev \
      libtool \
      libyaml-cpp-dev \
      llvm-3.8 \
      make \
      pkg-config \
      ocl-icd-libopencl1 \
      ocl-icd-opencl-dev \
      opencl-headers \
      sudo \
      tar \
      time \
      wget \
      xz-utils \
      zlib1g-dev && \
    apt-get clean

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-3.8 90 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.8 90 && \
    update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-3.8 90 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 90 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 90 && \
    update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-5 90 

WORKDIR /var/tmp/install-autoconf-archive
RUN wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2016.03.20.tar.xz && \
    tar -xf autoconf-archive-2016.03.20.tar.xz && \
    cd autoconf-archive-2016.03.20 && ./configure --prefix=/usr && make && make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/install-autoconf-archive
 
WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3.2/skye-0.3.2.tar.gz && \
    tar -xf skye-0.3.2.tar.gz && \
    cd skye-0.3.2 && \
    CXX=g++ CC=gcc ./configure && \
    make check && \
    make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-skye

WORKDIR /var/tmp/build-pocl
RUN wget -q http://portablecl.org/downloads/pocl-0.13.tar.gz && \
    tar -zxf pocl-0.13.tar.gz && \
    cd pocl-0.13 && ./configure && make && make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-pocl

WORKDIR /var/tmp/build-clFFT
RUN wget -q https://github.com/clMathLibraries/clFFT/archive/v2.12.2.tar.gz && \
    tar -xf v2.12.2.tar.gz && \
    cd clFFT-2.12.2/src && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local . && \
    make && \
    make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-clFFT

# ... use this to avoid setting LD_LIBRARY_PATH everywhere ...
RUN echo /usr/local/lib64 | tee /etc/ld.so.conf.d/clfft-2.12.2.conf && ldconfig

WORKDIR /var/tmp/build-boost-compute
RUN wget -q https://github.com/boostorg/compute/archive/boost-1.62.0.tar.gz && \
    tar -xf boost-1.62.0.tar.gz && \
    cd /var/tmp/build-boost-compute/compute-boost-1.62.0 && \
    cmake . && make && make DESTDIR=staging install && \
    cp -r staging/usr/local/include/compute/boost/compute.hpp /usr/include/boost/ && \
    cp -r staging/usr/local/include/compute/boost/compute/ /usr/include/boost/ && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-boost-compute

WORKDIR /root
