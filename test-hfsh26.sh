#! /bin/bash

if ! [[ -x hfsh26 ]]; then
    echo "hfsh26 executable does not exist"
    exit 1
fi

../tester/run-tests.sh $*


