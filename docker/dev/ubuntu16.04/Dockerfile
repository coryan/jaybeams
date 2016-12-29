FROM ubuntu:16.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update && apt-get install -y \
    automake \
    clang \
    clang-format \
    doxygen \
    findutils \
    g++ \
    gcc \
    git \
    lcov \
    libboost-all-dev \
    libfftw3-dev \
    libyaml-cpp-dev \
    make \
    tar \
    wget \
    xz-utils

WORKDIR /var/tmp/install-autoconf-archive
RUN wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2016.09.16.tar.xz
RUN tar -xf autoconf-archive-2016.09.16.tar.xz
RUN cd autoconf-archive-2016.09.16 && ./configure --prefix=/usr && make && make install

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3/skye-0.3.tar.gz
RUN tar -xf skye-0.3.tar.gz
RUN cd skye-0.3 && \
    CXX=g++ CC=gcc CPPFLAGS=-D_GLIBCXX_USE_CXX11_ABI=1 ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu && \
    make check && \
    make install

RUN apt-get install -y ocl-icd-libopencl1 opencl-headers clinfo ocl-icd-opencl-dev
RUN apt-get install -y cmake pkg-config llvm libhwloc-dev zlib1g-dev libclang-dev

WORKDIR /var/tmp/build-pocl
RUN wget http://portablecl.org/downloads/pocl-0.13.tar.gz
RUN tar -zxf pocl-0.13.tar.gz
RUN cd pocl-0.13 && ./configure && make && make install

WORKDIR /var/tmp/build-clFFT
RUN wget https://github.com/clMathLibraries/clFFT/archive/v2.12.2.tar.gz
RUN tar -xf v2.12.2.tar.gz
RUN cd clFFT-2.12.2/src && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/clFFT-2.12.1 . && \
    make && \
    make install

# ... use this to avoid settint LD_LIBRARY_PATH everywhere ...
RUN echo /usr/local/clFFT-2.12.1/lib64 >/etc/ld.so.conf.d/clfft-2.12.1.conf
RUN ldconfig

WORKDIR /var/tmp/build-boost-compute
RUN wget https://github.com/boostorg/compute/archive/boost-1.62.0.tar.gz
RUN tar -xf boost-1.62.0.tar.gz
RUN cd compute-boost-1.62.0 && \
    mkdir build && cd build && \
    CC=clang CXX=clang++ cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/boost-compute-1.62.0 \
      -DBOOST_COMPUTE_THREAD_SAFE=ON \
      -DCMAKE_CXX_FLAGS="-Wall -pedantic -Werror -Wno-variadic-macros -Wno-long-long -Wno-shadow -Qunused-arguments -DCL_VERSION_1_2 -Wno-deprecated-declarations -Wno-constant-logical-operand" .. && \
    make && make install

RUN clinfo

WORKDIR /root
RUN /bin/rm -fr /var/tmp/build-skye /var/tmp/install-autoconf-archive
RUN /bin/rm -fr /var/tmp/build-clFFT /var/tmp/build-boost-compute
