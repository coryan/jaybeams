#!/bin/bash

tag=$(date +%Y%m%d%H%M)
bindir=$(dirname $0)
for target in fedora{24,25} ubuntu{14.04,16.04}; do
    echo ================================================================
    echo
    echo Building ${target?} $(date)
    echo
    echo ================================================================
    sudo docker build -t coryan/jaybeamsdev-${target?}:${tag?} ${bindir?}/${target?}
done
