# TimeSwipe Library

### Build package for Arch
docker image timeswipe:arch should be available created by command:
```
docker build -t timeswipe:arch -f Dockerfile.arch .
```

build package command:
```
git submodule update --init
docker run --rm -v "$PWD/..":/usr/src  timeswipe:arch /usr/src/driver/build_arch.sh
```

Output package file is `timeswipe-x.x.x-x-any.pkg.tar.xz`

### Install package for Arch
install with command:
```
sudo pacman -U timeswipe-x.x.x-x-any.pkg.tar.xz
```

uninstall with command:
```
sudo pacman -R timeswipe
```

### Build package for Debian/Ubuntu
docker image timeswipe:arch should be available created by command:
```
docker build -t timeswipe:deb -f Dockerfile.ubuntu .
```

build package command:
```
git submodule update --init
docker run --rm -v "$PWD/..":/usr/src  timeswipe:deb /usr/src/driver/build_deb.sh
```

Output package file is `timeswipe_x.x.x.deb`

### Install package for Debian/Ubuntu
install with command:
```
sudo dpkg -i timeswipe_x.x.x.deb
```

uninstall with command:
```
sudo dpkg -r timeswipe
```

### Applications development

Cross-compiler aarch64 must be available in development environment

for ubuntu:
```
sudo apt install g++-aarch64-linux-gnu g++-arm-linux-gnueabihf libboost-all-dev
```

for Arch:
```
sudo pacman -Sy aarch64-linux-gnu-gcc arm-none-eabi-binutils community/arm-none-eabi-gcc extra/boost-libs extra/boost
```

for target platform:
```
cd driver
mkdir build
cd build
cmake ..
make
```

for OSX:
[README.md](contrib/OSX/README.md)

install on target platform:
```
sudo make install
```

Library package should be installed:

`timeswipe-x.x.x-x-any.pkg.tar.xz` for Arch

`timeswipe_x.x.x.aarch64.deb` for Ubuntu aarch64
`timeswipe_x.x.x.armv7l.deb` for Ubuntu/Raspbian armv7l

There is a DataLogging sample application with cmake configuration in ./examples/DataLogging directory:
```
cd examples/DataLogging
mkdir build
cd build
cmake ..
make
```

DataLogging cross-platform armv7l build:
```
cd examples/DataLogging
mkdir build
cd build
cmake -DARM32=True ..
make
```

### Python3 driver wrapper:
[README.md](python3/README.md)
