FROM fedora:25
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN dnf makecache && dnf install -y \
      autoconf \
      autoconf-archive \
      automake \
      boost \
      boost-devel \
      bzip2-devel \
      clang \
      clinfo \
      cmake \
      compiler-rt \
      doxygen \
      fftw-devel \
      findutils \
      gcc-c++ \
      git \
      lshw \
      make \
      ocl-icd-devel \
      opencl-headers \
      patch \
      pocl-devel \
      sudo \
      tar \
      time \
      valgrind \
      wget \
      yaml-cpp-devel \
      zlib-devel && \
    dnf clean all

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3.2/skye-0.3.2.tar.gz && \
    tar -xf skye-0.3.2.tar.gz && \
    cd skye-0.3.2 && \
    CXX=g++ CC=gcc ./configure && \
    make check && \
    make install && \
    cd .. && /bin/rm -fr /var/tmp/build-skye

WORKDIR /var/tmp/build-clFFT
RUN wget -q https://github.com/clMathLibraries/clFFT/archive/v2.12.2.tar.gz && \
    tar -xf v2.12.2.tar.gz && \
    cd clFFT-2.12.2/src && \
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/clFFT-2.12.2 . && \
    make && \
    make install && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-clFFT

# ... use this to avoid setting LD_LIBRARY_PATH everywhere ...
RUN echo /usr/local/clFFT-2.12.2/lib64 | tee /etc/ld.so.conf.d/clfft-2.12.2.conf && \
    ldconfig

WORKDIR /var/tmp/build-boost-compute
RUN wget -q https://github.com/boostorg/compute/archive/boost-1.62.0.tar.gz && \
    tar -xf boost-1.62.0.tar.gz && \
    cd compute-boost-1.62.0 && \
    cmake . && make && make DESTDIR=staging install && \
    cp -r staging/usr/local/include/compute/boost/compute.hpp /usr/include/boost/ && \
    cp -r staging/usr/local/include/compute/boost/compute/ /usr/include/boost/ && \
    cd /var/tmp && /bin/rm -fr /var/tmp/build-boost-compute

WORKDIR /root

# Boost has deprecated a few headers, warns about it, and other parts of Boost still
# use them:
#   https://svn.boost.org/trac/boost/ticket/11860
#   https://github.com/boostorg/iostreams/pull/24
#
# Until 1.62.0 or so is released, we simply remove the wawrnings from Boost.
# I could just remove them all:
#    RUN find /usr/include/boost/ -type f -exec sed -i '/pragma message.*deprecated/d' {} \;
# but I have chosen to be more surgical about it:
RUN sed -i '/pragma message.*deprecated/d' \
  /usr/include/boost/type_traits/detail/template_arity_spec.hpp \
  /usr/include/boost/type_traits/detail/bool_trait_def.hpp
