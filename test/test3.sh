#!/bin/sh
cd $1
git clone https://github.com/OSH-2018/3-NiceKingWei.git
cd 3-NiceKingWei
mkdir build
cd build
cmake3 ../
time make -j