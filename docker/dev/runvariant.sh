#!/bin/sh

if [ ! -d .git ]; then
    echo "You should run this from the top-level git working copy"
    exit 0
fi

if git remote -v | grep -q jaybeams.git; then
    /bin/true
else
    echo "This is intended to run in a jaybeams working copy"
    exit 0
fi

echo "This script may prompt you for your sudo password to connect to the docker daemon"

dockerfile=$1
if [ -d $dockerfile -a -f $dockerfile/Dockerfile ]; then
    dockerfile=$dockerfile/Dockerfile
fi

if [ ! -r $dockerfile -o ! -f $dockerfile ]; then
    echo "Usage: $0 <dockerfile>"
    exit 1
fi

if [ "x$USER" = "x" ]; then
    USER=$(whoami)
fi

if [ "x$UID" = "x" ]; then
    UID=$(id -u)
fi

if [ "x$USER" = "x" -o "x$UID" = "x" ]; then
    echo "USER or UID not set, script needs fixing for your platform"
    exit 1
fi

image=$( (cat $dockerfile; cat <<__EOF__
ARG user
ARG uid

RUN grep -q Ubuntu /etc/lsb-release && apt-get update && apt-get install -y lshw sudo time || :
RUN grep -q Fedora /etc/fedora-release && dnf update && dnf install -y lshw sudo time || :
RUN echo \$user ALL=NOPASSWD: ALL >>/etc/sudoers

WORKDIR /root
RUN useradd -m -u \$uid \$user
VOLUME /home/\$user/jaybeams

USER \$user
WORKDIR /home/\$user/jaybeams
__EOF__
) | sudo docker build -q --build-arg user=$USER --build-arg uid=$UID -)

echo "Created image $image, running it"

sudo docker run --rm -it -v $PWD:/home/$USER/jaybeams $image /bin/bash


