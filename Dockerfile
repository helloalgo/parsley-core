FROM ubuntu:18.04

RUN apt-get update -y && apt-get install -y gcc g++ default-jre default-jdk python3 python3-pip wget make cppcheck valgrind
RUN apt-get install -y nano git vim
RUN apt-get -y install software-properties-common
RUN add-apt-repository ppa:longsleep/golang-backports
RUN apt-get -y update
RUN apt-get -y install golang-1.13-go seccomp libseccomp-dev

EXPOSE 22/tcp 22/udp
EXPOSE 8080/tcp 8080/udp
EXPOSE 3000/tcp 3000/udp

CMD ["/bin/bash"]
