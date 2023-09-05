#!/bin/bash
clear
set -e
set +x

rm -rf build
mkdir build
rm -rf wav
mkdir wav
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=1 ../test/
make -j8

# Man
./test ../input.wav ../wav/output_man.wav -pitch=-4.0 -speech
# Woman
./test ../input.wav ../wav/output_woman.wav -pitch=5.0 -speech
# Cartoon
./test ../input.wav ../wav/output_cartoon.wav -pitch=6.5 -speech