#! /bin/bash
mkdir -p build
cd build
cmake -G"Unix Makefiles" ..
make -j $(nproc)

