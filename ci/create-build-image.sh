#!/bin/sh

# On the Travis server, /bin/sh does not define UID, so we define it.
if [ "x$UID"  = "x" ]; then
    UID=$(id -u)
fi

docker build -q - <<__EOF__
FROM ${IMAGE?}

WORKDIR /
RUN useradd -m -u ${UID?} ${USER?}

VOLUME /home/${USER?}/jaybeams

USER ${USER?}
WORKDIR /home/${USER?}/jaybeams
__EOF__

