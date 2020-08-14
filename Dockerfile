FROM ubuntu:18.04

RUN apt-get update && apt-get install -y gcc g++ coreutils man-db manpages-posix
ADD . /src
RUN cd /src && g++ main.cpp -o core -lpthread
WORKDIR /src

ENTRYPOINT /bin/bash