#!/bin/bash
cd `dirname $0`

build_dir=cmake_deb64
mkdir $build_dir || exit
cd $build_dir
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
destdir="$PWD"/timeswipe_`cat ../version`.aarch64
make VERBOSE=1 DESTDIR=$destdir install
cp -p -r DEBIAN $destdir
dpkg-deb --build $destdir && mv *.deb .. && cd .. && rm -rf $build_dir

build_dir=cmake_deb32
mkdir $build_dir || exit
cd $build_dir
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DARM32=True ..
make
destdir="$PWD"/timeswipe_`cat ../version`.armv7l
make VERBOSE=1 DESTDIR=$destdir install
cp -p -r DEBIAN $destdir
dpkg-deb --build $destdir && mv *.deb .. && cd .. && rm -rf $build_dir
