#!/bin/sh

# Exit on the first error ...
set -e

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
if [ -z "$dockerfile" ]; then
    echo "Missing docker directory (or Dockerfile) argument"
    exit 1
fi

if [ -d ${dockerfile?} -a -f ${dockerfile?}/Dockerfile ]; then
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

variant=$(basename $(dirname $dockerfile))

image=$(
    (cat <<__EOF__
FROM coryan/jaybeamsdev-${variant?}:latest
ARG user
ARG uid
VOLUME /home/\$user/jaybeams

WORKDIR /root
RUN useradd -m -u \$uid \$user
RUN echo \$user ALL=NOPASSWD: ALL >>/etc/sudoers
RUN chown \$user /home/\$user

USER \$user
WORKDIR /home/\$user/jaybeams
__EOF__
    ) | sudo docker build -q --build-arg user=$USER --build-arg uid=$UID - )

echo "Created image $image, running it"

sudo docker run --rm -it -v $PWD:/home/$USER/jaybeams \
     --env CXX=g++ --env CXXFLAGS="-g -O0" --env VARIANT=${variant?} --env USER=${USER?} \
     ${image?} docker/dev/stage-variant-helper.sh

cp docker/runtime/${variant?}/Dockerfile staging/${variant?}
sudo docker build -t coryan/jaybeams-runtime-${variant?}:latest staging/${variant?}
cp docker/analysis/Dockerfile staging/${variant?}/Dockerfile.analysis
sudo docker build -t coryan/jaybeams-analysis:latest -f staging/${variant?}/Dockerfile.analysis staging/${variant?}

exit 0

