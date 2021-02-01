# PANDA TimeSwipe Driver

The driver is a middleware between TimeSwipe board and Raspberry Pi. Supported
operating systems are Raspbian Buster and Arch Linux ARMv8 AArch64.

## Installing a Pre-Built Driver

The simplest way to install the TimeSwipe Driver is to download the prebuilt
packages from the [Release Section](https://github.com/panda-official/TimeSwipe/releases).

### Raspbian Buster

A downloaded package can be installed with `dpkg` like this:

```
sudo dpkg -i timeswipe_package.deb
```

or uninstalled like that:

```
sudo dpkg -r timeswipe
```

### Arch Linux ARMv8 AArch64

A downloaded package can be installed with `pacman` like this:

```
sudo pacman -U timeswipe_X.Y.Z-any.pkg.tar.xz
```

or uninstalled like that:

```
sudo pacman -R timeswipe
```

## Compiling the Driver

This section describes how to build the driver on Raspberry Pi.

### Compiling the driver on Raspbian Buster

Install the required packages:

```
sudo apt update
sudo apt install cmake git libboost-dev
```

Clone the TimeSwipe repository:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Build and install the driver:

```
cd timeswipe
mkdir build && cd build
cmake ..
cmake --build . --parallel
sudo make install
```

To create a package type:

```
dpkg-deb --build .
```

### Compiling the driver on Arch Linux ARMv8 AArch64

Update the system and install the required packages:

```
sudo pacman-key --init
sudo pacman-key --populate archlinuxarm
sudo pacman -Syu
sudo pacman -S git cmake make gcc boost-libs boost pkgconfig
```

Clone the TimeSwipe repository:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Build and install the driver:

```
cd timeswipe
mkdir build && cd build
cmake ..
cmake --build . --parallel
sudo make install
```

To create a package type:

```
sudo pacman -S fakeroot
makepkg
```

## Cross-compiling the Driver

This section shows how you can build the TimeSwipe driver on a computer other
than a Raspberry Pi. A cross-compilator required in order to cross-compile the
driver. Also, currently, Boost must be cross-compiled also.

### Cross-compiling Boost

Download the latest version of Boost. In the Boost folder run `./bootstrap`.
In the generated `project-config.jam` file, replace the line beginning with
`using gcc` with `using gcc : arm : arm-linux-gnueabihf-g++ ;` (for ARM32) or
`using gcc : arm : aarch64-linux-gnu-g++ ;` (for ARM64). Now compile and install
with `./b2 link=static runtime-link=static install toolset=gcc-arm --prefix=/opt/arm`
(for ARM32) or `./b2 link=static runtime-link=static install toolset=gcc-arm --prefix=/opt/arm64`
(for ARM64).

Please note that we're working to stop the dependency on Boost!

### Cross-compiling the Driver on Ubuntu Linux

Install the required packages:

```
sudo apt update
sudo apt install gcc g++ gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
                 gcc-aarch64-linux-gnu g++-aarch64-linux-gnu cmake git pkg-config
```

Clone the TimeSwipe repository:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Cross-compile Boost as [described above](#cross-compiling-boost).

Build the driver:

```
cd timeswipe
mkdir build && cd build
cmake -DPANDA_BUILD_ARM=On ..
cmake --build . --parallel
sudo make install
```

Please note, `-DPANDA_BUILD_ARM=On` can be replaced with `-DPANDA_BUILD_ARM64=On`
in order to build for ARM64.

### Cross-compiling the Driver on macOS

For cross-compilation on macOS, `ct-ng` and some extra dependencies are needed. If
brew is not installed so far, please follow the [instructions](https://brew.sh).

Install the required packages:

```
brew install cmake
brew install crosstool-ng help2man bison
```

Use `Disk Utility` to create an APFS (case-sensitive) disk named
`xtool-build-env`. Then change the limit of file descriptors:

```
ulimit -n 1024
```

Clone the TimeSwipe repository:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Copy crosstool-NG configuration file:

```
cp timeswipe/contrib/OSX/config_osx /Volumes/xtool-build-env/.config
```

Now build the toolchain (it will take a while):

```
cd /Volumes/xtool-build-env
ct-ng build
```

After the toolchain built successfully, change the PATH variable:

```
export PATH=/Volumes/xtool-build-env/aarch64-rpi3-linux-gnu/bin:$PATH
```

Cross-compile Boost as [described above](#cross-compiling-boost).

Navigate to the project directory and build:

```
cd timeswipe
mkdir build && cd build
cmake -DPANDA_BUILD_ARM64=On ..
cmake --build . --parallel
sudo make install
```

## Build the Driver emulator on Debian-based OS

Install the required packages:

```
sudo apt install gcc g++ libboost-dev cmake git pkg-config
```

Clone the TimeSwipe repository:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Build the driver in emulation mode:

```
cd timeswipe
mkdir build && cd build
cmake -DPANDA_BUILD_FIRMWARE_EMU=On ..
cmake --build . --parallel
```

After the successive build `datlog` example can be runned:

```
./datlog --config datlog.json
```
