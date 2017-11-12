FROM fedora:24
MAINTAINER "Carlos O'Ryan <coryan@users.noreply.github.com>"

RUN dnf makecache && dnf install -y \
    binutils \
    boost \
    bzip2 \
    fftw \
    gawk \
    kernel-tools \
    lshw \
    ocl-icd \
    procps-ng \
    sudo \
    time \
    which \
    yaml-cpp \
    zlib && dnf clean all

COPY local/bin /opt/jaybeams/bin/
COPY local/lib /opt/jaybeams/lib/
COPY local/include /opt/jaybeams/include/
