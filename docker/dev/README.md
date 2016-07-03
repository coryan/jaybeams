## Creating Docker Images for the Development Environment

This directory contains a number of Dockerfiles to create a Jaybeams
development environment on different operating systems.  It also
includes Dockerfile fragments to test those images and verify they are
actually usable.

### Building images

Just use docker, e.g.

     docker build -t coryan/jaybeamsdev:ubuntun14.04 docker/dev/ubuntu14.04
     docker build -t coryan/jaybeamsdev:ubuntun16.04 docker/dev/ubuntu16.04

### Testing images

You can test the development environment using the `Dockerfile.test` fragment:

    cd docker/dev/ubuntu14.04
    cat Dockerfile Dockerfile.test | docker build  -

or

    cd docker/dev/ubuntu16.04
    cat Dockerfile Dockerfile.test | docker build  -
