FROM ubuntu:18.04

RUN apt-get update && apt-get install -y gcc g++ coreutils man-db manpages-posix make libseccomp-dev gdb strace nano python3 openjdk-8-jdk
ADD . /src
RUN cd /src && make all
WORKDIR /src/bin

ENTRYPOINT /bin/bash