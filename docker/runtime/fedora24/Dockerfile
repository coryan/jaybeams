FROM fedora:24
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN dnf update -y && dnf install -y \
    boost \
    fftw \
    ocl-icd \
    yaml-cpp
RUN dnf clean all

# pocl installs a lot of development tools, think about that some more.
