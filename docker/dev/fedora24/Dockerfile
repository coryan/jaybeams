FROM fedora:24
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

#'
RUN dnf makecache && dnf install -y \
      autoconf \
      autoconf-archive \
      automake \
      boost \
      boost-devel \
      boost-static \
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
      golang \
      libtool \
      lshw \
      make \
      ocl-icd-devel \
      opencl-headers \
      openssl \
      openssl-devel \
      openssl-static \
      patch \
      pocl-devel \
      shtool \
      sudo \
      tar \
      time \
      unzip \
      wget \
      yaml-cpp-devel \
      zlib-devel && \
    dnf clean all

WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3.2/skye-0.3.2.tar.gz && \
    tar -xf skye-0.3.2.tar.gz && \
    (cd skye-0.3.2 && \
      CXX=g++ CC=gcc ./configure && \
      make check && \
      make install) && \
    /bin/rm -fr /var/tmp/build-skye

WORKDIR /var/tmp/build-clFFT
RUN wget -q https://github.com/clMathLibraries/clFFT/archive/v2.12.2.tar.gz && \
    tar -xf v2.12.2.tar.gz && \
    (cd clFFT-2.12.2/src && \
      cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/clFFT-2.12.2 . && \
      make && \
      make install) && \
    /bin/rm -fr /var/tmp/build-clFFT

# ... use this to avoid setting LD_LIBRARY_PATH everywhere ...
RUN echo /usr/local/clFFT-2.12.2/lib64 | \
    tee /etc/ld.so.conf.d/clfft-2.12.2.conf && \
    ldconfig

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

#
# This is awful, the distribution of gRPC is very Ubuntu specific.
# We need to manually set the environment variables that select the C
# and C++ compiler (as well as linkers).
#
WORKDIR /var/tmp/build-grpc
RUN git clone --depth 10 --branch v1.3.4 https://github.com/grpc/grpc.git && \
  (cd /var/tmp/build-grpc/grpc/ && \
    git submodule update --init && \
    env LDXX="g++ -std=c++14" LD=gcc CXX="g++ -std=c++14" CC=gcc \
      make && \
    env LDXX="g++ -std=c++14" LD=gcc CXX="g++ -std=c++14" CC=gcc \
      make install && \
    env LDXX="g++ -std=c++14" LD=gcc CXX="g++ -std=c++14" CC=gcc \
      make -C third_party/protobuf install && \
    echo /usr/local/lib >/etc/ld.so.conf.d/local.conf && ldconfig && \
    find /usr/local -type f -name '*.la' -exec rm -f {} \;) && \
  /bin/rm -fr /var/tmp/build-grpc 

WORKDIR /var/tmp/install-etcd
RUN wget -q https://github.com/coreos/etcd/releases/download/v3.2.1/etcd-v3.2.1-linux-amd64.tar.gz && \
  tar -xf etcd-v3.2.1-linux-amd64.tar.gz && \
  cp etcd-v3.2.1-linux-amd64/etcd /usr/bin && \
  cp etcd-v3.2.1-linux-amd64/etcdctl /usr/bin && \
  /bin/rm -fr /var/tmp/install-etcd
# TODO() this should be installed in that first "dnf install", need to
# debug the image first.
RUN dnf makecache && dnf install -y python && dnf clean all

# Boost has deprecated a few headers, warns about it, and other parts
# of Boost still use them:
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

WORKDIR /root
