#!/bin/bash

export PATH="/mingw64/bin"
cd ${APPVEYOR_BUILD_FOLDER}

# FIXME: get the number of cores
JCORES="-j4"

./.integration/ci-run-tests.sh $JCORES
exit $?
