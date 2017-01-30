FROM fedora:23
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN dnf update -y && dnf install -y \
    boost \
    bzip2 \
    fftw \
    ocl-icd \
    yaml-cpp \
    zlib

RUN dnf clean all
