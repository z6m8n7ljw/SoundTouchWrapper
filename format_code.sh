#!/bin/bash

function format_file() {
    file=$1
    if [ "${file##*.}"x = "h"x ] ||
       [ "${file##*.}"x = "hpp"x ] ||
       [ "${file##*.}"x = "c"x ] ||
       [ "${file##*.}"x = "cpp"x ] ||
       [ "${file##*.}"x = "m"x ] ||
       [ "${file##*.}"x = "mm"x ];
    then
        clang-format -style="{BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 120}" -i $file
    fi
}

function format_directorie() {
    cd $1

    for file in $(find . -type f -name '*.h' -or -name '*.hpp' -or -name '*.c' -or -name '*.cpp' -or -name '*.m' -or -name '*.mm')
    do
        clang-format -style="{BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 120}" -i $file
    done

    cd -
}

function format() {
    if [ -d $1 ]; then
        format_directorie $1
    elif [ -f $1 ]; then
        format_file $1
    fi
}

if [ $# -eq 0 ] ; then
    format $(pwd)/include
    format $(pwd)/src
    format $(pwd)/test
else
    for i in $@; do
        format $(pwd)/$i
    done
fi
