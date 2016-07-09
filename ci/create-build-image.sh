#!/bin/sh

(cat <<__EOF__
FROM coryan/${IMAGE}
ARG user
ARG uid

WORKDIR /
RUN useradd -m -u \$uid \$user

VOLUME /home/\$user/jaybeams

USER \$user
WORKDIR /home/\$user/jaybeams
__EOF__
) | docker build -q --build-arg user=$USER --build-arg uid=$UID -

