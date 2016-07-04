FROM ubuntu:14.04
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN apt-get update
RUN apt-get install -y software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update
RUN apt-get install -y \
    automake \
    doxygen \
    g++-5 \
    gcc-5 \
    clang-3.6 \
    git \
    lcov \
    libboost1.55-all-dev \
    libfftw3-dev \
    libyaml-cpp-dev \
    make \
    tar \
    wget \
    xz-utils

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-3.6 90
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.6 90
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 90
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 90
RUN update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-5 90

WORKDIR /var/tmp/install-autoconf-archive
RUN wget -q http://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-2016.03.20.tar.xz		
RUN tar -xf autoconf-archive-2016.03.20.tar.xz		
RUN cd autoconf-archive-2016.03.20 && ./configure --prefix=/usr && make && make install
 
WORKDIR /var/tmp/build-skye
RUN wget -q https://github.com/coryan/Skye/releases/download/v0.3/skye-0.3.tar.gz
RUN tar -xf skye-0.3.tar.gz
RUN cd skye-0.3 && \
    CXX=g++ CC=gcc ./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu && \
    make check && \
    make install

WORKDIR /root
RUN /bin/rm -fr /var/tmp/build-skye /var/tmp/install-autoconf-archive

