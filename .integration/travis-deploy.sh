#!/bin/bash

# This script is called when Travis-CI succeeded its job.

### Deploy the unit tests coverage report
RAPPORT=build/rapport.info
cd tests
lcov --quiet --directory .. -c -o $RAPPORT
lcov --quiet --remove $RAPPORT '/usr*' 'external/*' 'tests/*' -o $RAPPORT
coveralls-lcov --source-encoding=ISO-8859-1 $RAPPORT
cd ..
