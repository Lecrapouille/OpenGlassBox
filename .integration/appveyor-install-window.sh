#!/bin/bash

### Install global packages

packages=""
function mingw_packages
{
    arch=x86_64
    if [ "$PLATFORM" == "x86" ];
    then
        arch=i686
    fi

    prefix="mingw-w64-${arch}-"
    for i in $1
    do
        packages="$prefix""$i "$packages
    done
}

mingw_packages "ninja"
sh -c "pacman -S --noconfirm git cmake make bc gcc $packages"

### Install the newer lcov (>= 1.14) because default lcov 1.13 does not support gcc >= 8
### Use the correct gcov according to the GCC used
sh -c "pacman -S --noconfirm perl-JSON perl-PerlIO-gzip"
git clone https://github.com/linux-test-project/lcov.git --depth=1
(cd lcov && make install)

### Install google test and google mock
git clone https://github.com/google/googletest.git --depth=1
(cd googletest && CXX=$COMPILER cmake . && make install)
