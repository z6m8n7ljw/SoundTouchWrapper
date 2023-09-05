#!/bin/bash
clear
set -e
set +x

rm -rf build
mkdir build
mkdir -p build/valgrind_log
rm -rf wav
mkdir wav
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=1 ../test/
make -j8

valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_log/test.log ./test ../input.wav ../wav/output_man.wav -pitch=-4.0 -speech
cat valgrind_log/test.log
echo -e "\n"