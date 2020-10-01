#!/bin/bash

# Detect number of CPU cores for faster the compilation.
# TODO Mac OS X, Windows
CORES=`grep -c '^processor' /proc/cpuinfo` 2> /dev/null
if [ "$CORES" == "" ]; then
    CORES=2
fi
JCORES="-j"$(( CORES * 2 ))
echo "I detected $CORES CPUs I'll use $JCORES"

./.integration/ci-run-tests.sh $JCORES
exit $?
