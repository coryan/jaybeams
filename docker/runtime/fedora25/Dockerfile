FROM fedora:25
MAINTAINER Carlos O'Ryan <coryan@users.noreply.github.com>

RUN dnf makecache && dnf install -y \
    boost \
    bzip2 \
    fftw \
    gawk \
    lshw \
    ocl-icd \
    procps-ng \
    sudo \
    time \
    which \
    yaml-cpp \
    zlib && dnf clean all

COPY bin /opt/jaybeams/bin/
COPY lib /opt/jaybeams/lib/
COPY include /opt/jaybeams/include/
