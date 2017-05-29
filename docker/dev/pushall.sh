#!/bin/bash

tag=$1

base=$(basename $0)
if [ "x${tag}" = "x" ]; then
    echo "Usage: ${base?} <tag>"
    echo "  where <tag> is usually assigned by buildall.sh"
    exit 1
fi

for target in fedora{24,25} ubuntu16.04; do
    echo ================================================================
    echo
    echo Pushing ${target?} $(date)
    echo
    echo ================================================================
    sudo docker tag coryan/jaybeamsdev-${target?}:${tag?} coryan/jaybeamsdev-${target?}:latest
    sudo docker push coryan/jaybeamsdev-${target?}:${tag?}
    sudo docker push coryan/jaybeamsdev-${target?}:latest
done
