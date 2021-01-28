#!/bin/bash
cd `dirname $0`
build_dir=cmake_arch
mkdir $build_dir || exit
cd $build_dir
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
destdir="$PWD"/timeswipe_`cat ../version`

cd .. && chown -R build $build_dir && cd $build_dir && sudo -u build makepkg && mv *.xz .. && cd .. && rm -rf $build_dir
