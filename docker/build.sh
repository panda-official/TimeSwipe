#!/bin/bash

source_dir="${build_root}/timeswipe"
build_dir="${source_dir}/build"
firmware_source_dir="${source_dir}/firmware/src"
firmware_build_dir="${source_dir}/firmware_build"

git clone --recursive https://github.com/panda-official/TimeSwipe.git ${source_dir}
cd ${source_dir} || exit
git fetch origin pull/111/head:pr111
git checkout pr111

echo Building the driver library

mkdir -p "${build_dir}"
cd "${build_dir}" || exit
cmake -DPANDA_BUILD_ARM=1 -DDMS=0 ${source_dir}
cmake --build . --parallel `nproc`

echo Building the driver

mkdir -p "${firmware_build_dir}"
cd "${firmware_build_dir}" || exit
cmake ${firmware_source_dir}
cmake --build . --parallel `nproc`

if [ $? -eq 0 ]; then
  echo Successfully built!
  echo Please use \"docker cp \<image-hash\>:${build_dir}/tsbe/build \<dest-dir\>\" command to extract to host system.
  echo Press CTRL-D to stop the image. \(Build results will be lost!\)
  while read; do
    echo Press CTRL-D to stop the image. \(Build results will be lost!\)
  done
fi
