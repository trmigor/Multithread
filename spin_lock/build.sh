#!/bin/bash
DIRNAME=`dirname "$0"`
make -C $DIRNAME --always-make out-64 || exit -1
$DIRNAME/out-64
exit $?
