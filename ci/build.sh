#!/bin/bash

./autogen.sh &&
    mkdir -p build &&
    cd build &&
    ../configure &&
    make
