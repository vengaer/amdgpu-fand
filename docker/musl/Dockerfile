FROM alpine:edge
LABEL maintainer="vilhelm.engstrom@tuta.io"

RUN apk update  -v && \
    apk upgrade -v && \
    apk add -v --no-cache make clang gcc git graphviz musl-dev linux-headers argp-standalone

ENV CC=gcc

COPY . /fand
WORKDIR /fand
