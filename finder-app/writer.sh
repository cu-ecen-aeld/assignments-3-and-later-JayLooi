#!/bin/bash

ARGS=("writefile" "writestr")
N_ARGS=${#ARGS[@]}

if [[ $# -lt $N_ARGS ]]
then
    echo "ERROR: Not enough positional argument"
    for ((i=$#; i<$N_ARGS; ++i))
    do
        echo "Missing argument $i : ${ARGS[$i]}"
    done
    exit 1
fi

writefile=$1
writestr=$2

filedir=${writefile%/*}

if [[ $filedir == "" ]]
then
    filedir="/"
elif [[ $filedir == $writefile ]]
then
    filedir="."
fi

if [[ ! -d $filedir ]]
then
    mkdir -p $filedir 2>/dev/null
fi

2>/dev/null echo $writestr > $writefile

if [[ $? -ne 0 ]]
then
    echo "ERROR: Create $writefile failed"
    exit 1
fi
