#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64:/usr/local/lib64:/usr/local/lib/
sudo apt-get -qq update

### Install global packages
sudo apt-get install -y git cmake make libdw-dev pkg-config bc libsdl2-dev libsdl2-image-dev

### Install the code coverage reporter (GitHub service coveralls)
sudo gem install coveralls-lcov

### Install the newer lcov (>= 1.14) because default lcov 1.13 does not support gcc >= 8
### Use the correct gcov according to the GCC used
sudo apt-get install -y libjson-perl libperlio-gzip-perl
git clone https://github.com/linux-test-project/lcov.git --depth=1
(cd lcov && sudo make install)
(cd /usr/bin && sudo ln -sf $GCOV gcov)

### Install google test and google mock
wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz
tar xf release-1.10.0.tar.gz
(cd googletest-release-1.10.0
 CXX=$COMPILER cmake -DBUILD_SHARED_LIBS=ON .
 sudo make install
)
