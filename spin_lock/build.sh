#!/bin/bash
DIRNAME=`dirname "$0"`
make -C $DIRNAME out-64 || exit -1
$DIRNAME/build/out-64
exit $?
