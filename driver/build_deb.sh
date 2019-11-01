#!/bin/bash
cd `dirname $0`
build_dir=cmake_deb
mkdir $build_dir || exit
cd $build_dir
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
destdir="$PWD"/timeswipe_`cat ../version`
make DESTDIR=$destdir install

cp -p -r DEBIAN $destdir
dpkg-deb --build $destdir && mv *.deb .. && cd .. && rm -rf $build_dir
