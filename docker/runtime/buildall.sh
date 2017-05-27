#!/bin/bash

tag=$(date +%Y%m%d%H%M)
bindir=$(dirname $0)
for target in fedora{24,25} ubuntu16.04; do
    echo ================================================================
    echo
    echo Building ${target?} $(date)
    echo
    echo ================================================================
    sudo docker build -t coryan/jaybeams-runtime-${target?}:${tag?} ${bindir?}/${target?} || break
done
