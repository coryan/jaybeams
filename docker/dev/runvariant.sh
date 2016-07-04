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

dockerfile=$1
if [ -d $dockerfile -a -f $dockerfile/Dockerfile ]; then
    dockerfile=$dockerfile/Dockerfile
fi

if [ ! -r $dockerfile -o ! -f $dockerfile ]; then
    echo "Usage: $0 <dockerfile>"
    exit 1
fi


image=$((cat $dockerfile; cat <<__EOF__
ARG user
ARG uid

WORKDIR /
RUN useradd -m -u \$uid \$user

USER \$user
WORKDIR /home/\$user
__EOF__
) | docker build -q --build-arg user=$USER --build-arg uid=$UID -)

echo "Created image $image, running it"

docker run --rm -it -v $PWD:/home/$USER/jaybeams $image /bin/bash


