#!/usr/bin/env bash

pushd ../src/
autoreconf
automake --add-missing
popd

pushd ../build/
rm Makefile
../src/configure --prefix="`pwd`/.."
make CXXFLAGS="-ggdb -Wall -O2" clean install
popd
