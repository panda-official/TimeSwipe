# TimeSwipe Driver

The driver is used for the communication between the TimeSwipe board and the Raspberry Pi.
Supported operating systems on the Raspberry Pi are Raspbian Buster (and to an extent, any Debian-based OS), and Arch Linux.
You can also cross-compile the driver on other host systems.
The `examples` folder contains two example programs which use the driver.

## Installing a Pre-Built Driver

### Raspbian Buster

tbd.

### Arch Linux 64

tbd.

## Building the Driver

This section describes how to build the driver on your own Raspberry Pi.

### Raspbian Buster

On a vanilla installation of Raspbian Buster, you will need to install some packages:

```
sudo apt-get update
sudo apt-get install cmake git libboost-dev
```

You can then clone this repository, if you haven't done that already:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Navigate to the `driver` directory, build and install the driver:

```
cd timeswipe/driver
mkdir -p build
cd build
cmake ..
make -j$(nproc)
sudo make install
```

The TimeSwipe driver should now be installed on your system.


### Arch Linux 64

tbd.

## Cross-Compiling the Driver

### Ubuntu 18.04

### Arch Linux 64

### OSX

## Compiling Applications Using the Driver

This section describes how to compile the `DataLogging` example application using the TimeSwipe driver.

### Raspbian Buster

Navigate to the `DataLogging` directory:

```
cd timeswipe/driver/examples/DataLogging
```

You can then build the application with `cmake`:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

To run the data logging example from the `build` directory, execute the command:

```
sudo ./main --config ../config.json --input IEPE --output temp.txt
```

This will gather data according to the configuration file specified, from the `IEPE` inputs and will save the data in CSV format to the file `temp.txt`.

### Arch Linux 64

tbd.






# Legacy Information:

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
