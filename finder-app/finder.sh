#!/bin/bash

ARGS=("filesdir" "searchstr")
N_ARGS=${#ARGS[@]}

if [[ $# -lt $N_ARGS ]]
then
    echo "ERROR: Not enough positional arguments"
    for ((i=$#; i<$N_ARGS; ++i))
    do
        echo "Missing argument $i : ${ARGS[$i]}"
    done
    exit 1
fi

filesdir=$1
searchstr=$2

if [[ ! -d $filesdir ]]
then
    echo "ERROR: $filesdir is not a directory"
    exit 1
fi

results="$(grep -r -c $searchstr $filesdir)"
numOfFile=$(echo "$results" | grep -cE ':[[:digit:]]+$')
numOfMatchedLine=$(echo "$results" | grep -cE ':[1-9][[:digit:]]*$')

echo "The number of files are $numOfFile and the number of matching lines are $numOfMatchedLine"

