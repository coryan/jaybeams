# How to Setup your Workstation to Compile JayBeams

## Introduction

These are my notes on how to prepare your environment to build
JayBeams.  If you find a problem with the instructions, please submit
a patch or file a bug.

The continuous integration builds for JayBeams run inside [docker](https://docker.io), a single Dockerfile installs 
all necessary dependencies, compiles them, then compiles the code, runs the tests, verifies that the install rules 
work as expected and (in some cases), even verify that the code is formatted properly.

It is possible to use the same docker file to build your copy of the code, and this is probably the most reliable way
to get JayBeams compiled.  We will describe this first and then describe how to manually install all the dependencies.
 
## Running a Docker-based Build

To compile using a Ubuntu 16.04 image, with `gcc`, using the default configuration use:

```commandline
https://github.com/coryan/jaybeams.git
cd jaybeams
LINUX_BUILD=yes DISTRO=ubuntu DISTRO_VERSION=16.04 CXX=g++ CC=gcc ./ci/build-linux.sh
```

That's it.  This script will use your current checkout, install and build all the dependencies, and then build JayBeams.

The results of installing and building the dependencies are cached by the docker daemon, so your next build would be 
substantially faster.  You can also try using a different compiler, e.g.:

```commandline
LINUX_BUILD=yes DISTRO=ubuntu DISTRO_VERSION=16.04 CXX=clang++ CC=clang \
  ./ci/build-linux.sh
```

Or compile with different settings, e.g.:

```commandline
LINUX_BUILD=yes DISTRO=ubuntu DISTRO_VERSION=16.04 CXX=clang++ CC=clang \
  CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release" ./ci/build-linux.sh
```

While this is fast enough for continuous integration builds, it rebuilds all the JayBeams code each time, for quick 
development you may want to install the dependencies in your workstation and compile from the command-line or within 
your IDE.

## Installing on Fedora 25

We assume that you have superuser privileges in your workstation.  These instructions are largely copied from the 
`ci/Dockerfile.fedora` file, if you find an error please let me [know](https://github.com/coryan/jaybeams/issues), 
and consult that file for possible workarounds.

### Installing the Developement Tools

```commandline
sudo dnf makecache
sudo dnf install -y autoconf autoconf-archive automake boost boost-devel boost-static bzip2-devel clang clinfo \
  cmake compiler-rt curl doxygen fftw-devel findutils gcc-c++ git golang lcov libtool make lshw ocl-icd-devel \
  opencl-headers openssl openssl-devel openssl-static patch pocl-devel pkgconfig python shtool sudo tar time \
  unzip valgrind wget yaml-cpp-devel zlib-devel
dnf clean all
```

### Installing Additional Packages

#### clFFT (FFT library for OpenCL)

```commandline
mkdir /var/tmp/build-clFFT
cd /var/tmp/build-clFFT
wget -q https://github.com/clMathLibraries/clFFT/archive/v2.12.2.tar.gz
tar -xf v2.12.2.tar.gz
cd clFFT-2.12.2/src
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/clFFT-2.12.2 .
make
sudo make install
cd
/bin/rm -fr /var/tmp/build-clFFT
# ... use this to avoid settint LD_LIBRARY_PATH everywhere ...
echo /usr/local/clFFT-2.12.2/lib64 | sudo tee /etc/ld.so.conf.d/clfft-2.12.2.conf
sudo ldconfig
```

### Boost.Compute (C++ wrappers for OpenCL)

```commandline
wget -q https://github.com/boostorg/compute/archive/boost-1.62.0.tar.gz
tar -xf boost-1.62.0.tar.gz
cd compute-boost-1.62.0
cmake . && make && make DESTDIR=staging install
sudo cp -r staging/usr/local/include/compute/boost/compute.hpp /usr/include/boost/
sudo cp -r staging/usr/local/include/compute/boost/compute/ /usr/include/boost/
sudo sed -i '/pragma message.*deprecated/d' \
  /usr/include/boost/type_traits/detail/template_arity_spec.hpp \
  /usr/include/boost/type_traits/detail/bool_trait_def.hpp
```

### gRPC (a remote procedure call framework by Google)

```commandline
mkdir /var/tmp/build-grpc
cd /var/tmp/build-grpc
git clone --depth 10 --branch v1.6.x https://github.com/grpc/grpc.git && \
  (cd /var/tmp/build-grpc/grpc/ && \
    git submodule update --init && \
    env LDXX="g++ -std=c++14" LD=gcc CXX="g++ -std=c++14" CC=gcc \
      make && \
    env LDXX="g++ -std=c++14" LD=gcc CXX="g++ -std=c++14" CC=gcc \
      sudo make install && \
    env LDXX="g++ -std=c++14" LD=gcc CXX="g++ -std=c++14" CC=gcc \
      sudo make -C third_party/protobuf install && \
    echo /usr/local/lib >/etc/ld.so.conf.d/local.conf && ldconfig && \
    find /usr/local -type f -name '*.la' -exec rm -f {} \;)
cd
/bin/rm -fr /var/tmp/build-grpc
```

### etcd (a distributed lock service daemon)

This is only used in certain integration tests, but you should install it anyway:

```commandline
mkdir /var/tmp/install-etcd
cd /var/tmp/install-etcd
wget -q https://github.com/coreos/etcd/releases/download/v3.2.1/etcd-v3.2.1-linux-amd64.tar.gz && \
  tar -xf etcd-v3.2.1-linux-amd64.tar.gz && \
  sudo cp etcd-v3.2.1-linux-amd64/etcd /usr/bin && \
  sudo cp etcd-v3.2.1-linux-amd64/etcdctl /usr/bin
cd
/bin/rm -fr /var/tmp/install-etcd
```

### Checkout the Code and Build

This is the easy part:

```commandline
git clone https://github.com/coryan/jaybeams.git
cd jaybeams
mkdir build
cd build
cmake ..
make
```

To run the tests use the `test` make target:

```commandline
make test
```
