#!/bin/bash
git clone --recursive https://github.com/panda-official/TimeSwipe.git timeswipe

mkdir -p ${driver_build_dir}
cd ${driver_build_dir} || exit

cmake -DARM32=1 -DDMS=0 ${driver_source_dir}
cmake --build . --parallel `nproc`
if [ $? -eq 0 ]; then
  echo Successfully built!
  echo Please use \"docker cp \<image-hash\>:${driver_build_dir}/tsbe/build \<dest-dir\>\" command to extract to host system.
  echo Press CTRL-D to stop the image. \(Build results will be lost!\)
  while read; do
    echo Press CTRL-D to stop the image. \(Build results will be lost!\)
  done
fi
