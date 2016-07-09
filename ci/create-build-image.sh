#!/bin/sh

docker build -q - <<__EOF__
FROM coryan/${IMAGE?}
ARG user
ARG uid

WORKDIR /
RUN useradd -m -u ${UID?} ${USER?}

VOLUME /home/${USER?}/jaybeams

USER ${USER?}
WORKDIR /home/${USER?}/jaybeams
__EOF__

