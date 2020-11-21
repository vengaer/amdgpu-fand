FROM archlinux:latest
LABEL maintainer="vilhelm.engstrom@tuta.io"

RUN pacman -Syu --needed --noconfirm make clang gcc

ENV CC=gcc

COPY . /fand
WORKDIR /fand
