#!/bin/sh

set -e
mkdir -p bin

g++ -Icompiler compiler/mark.cpp \
    -Wall -Wextra \
    -std=c++23 \
    -g \
    -o bin/mark

g++ -Icompiler worker/worker.cpp \
    -Wall -Wextra \
    -std=c++23 \
    -g \
    -o bin/mark-worker

echo "built bin/mark and bin/mark-worker"
