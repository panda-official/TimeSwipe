# TimeSwipe Driver

The driver is used for the communication between the TimeSwipe board and the Raspberry Pi.
Supported operating systems on the Raspberry Pi are Raspbian Buster (and to an extent, any Debian-based OS), and Arch Linux ARMv8 AArch64.
You can also cross-compile the driver on other host systems.
The `examples` folder contains two example programs which use the driver.


## Installing a Pre-Built Driver

The simplest way to install the TimeSwipe Driver is to download the prebuilt packages.


### Raspbian Buster

If you are running Raspbian Buster (or any other Debian-based OS), simply download the package - you can find the latest in the [Release Section](https://github.com/panda-official/TimeSwipe/releases):

```
wget https://github.com/panda-official/TimeSwipe/releases/download/vX.Y.Z/timeswipe_X.Y.Z.armv7l.deb
```

Then simply install with:

```
sudo dpkg -i timeswipe_X.Y.Z.armv7l.deb
```

You can also uninstall the driver with the command:

```
sudo dpkg -r timeswipe
```


### Arch Linux ARMv8 AArch64

On Arch Linux, similarly download the appropriate package:

```
wget https://github.com/panda-official/TimeSwipe/releases/download/vX.Y.Z/timeswipe-X.Y.Z-1-any.pkg.tar.xz
```

And then as `root` install with:

```
pacman -U timeswipe_X.Y.Z-any.pkg.tar.xz
```

The package can be uninstalled with:

```
pacman -R timeswipe
```


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
You can optionally create your own Debian package with `dpkg-deb --build .`.


### Arch Linux ARMv8 AArch64

On a vanilla installation of Arch Linux, as `root` run the commands:

```
pacman-key --init
pacman-key --populate archlinuxarm
pacman -Syu
pacman -S make gcc boost-libs boost pkgconfig
```

to update your system and install the needed packages.

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
```

As `root` then install the driver:

```
make install
```

The TimeSwipe driver should now be installed on your system.
To optionally create your own `pkg.tar.xz` driver package, run `makepkg` from the `build` directory.


## Cross-Compiling the Driver

This section shows how you can build the TimeSwipe driver on a computer other than a Raspberry Pi.
Supported cross-compilation architectures are Ubuntu 18.04 and OSX.


### Ubuntu 18.04

To cross-compile on Ubuntu 18.04, you will need to install some extra packages:

```
sudo apt-get install gcc-arm-none-eabi libboost-dev gcc-aarch64-linux-gnu cmake git gcc g++-aarch64-linux-gnu g++ pkg-config
```

You can then clone this repository, if you haven't done that already:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Then simply navigate to the driver directory and build:

```
cd timeswipe/driver
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```


### OSX

For cross-compilation on OSX, `ct-ng` and some extra dependencies are needed. If brew is not installed so far, follow the [instructions](https://brew.sh).
In a terminal, enter the commands:

```
brew install crosstool-ng help2man bison
```

Using `Disk Utility` create an APFS (case-sensitive) disk named `xtool-build-env`.
Change the limit of file descriptors:

```
ulimit -n 1024
```

And copy the file `config_osx` from the `contrib/OSX` direcory to `/Volumes/xtool-build-env`, renaming the file in the process:

```
cp contrib/OSX/config_osx /Volumes/xtool-build-env/.config
```

Then you can build the toolchain:

```
cd /Volumes/xtool-build-env
ct-ng build
```

This will take a while.
Then, with `brew, install `boost` and verify the availability of its path:

```
brew install boost
brew --prefix boost
```

After the toolchain built successfully, change the PATH variable:

```
export PATH=/Volumes/xtool-build-env/aarch64-rpi3-linux-gnu/bin/:$PATH
```

You can then clone this repository, if you haven't done that already:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Then simply navigate to the driver directory and build:

```
cd timeswipe/driver
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

## Build the Driver emulator for Ubuntu 18.04 x86_64

To build on Ubuntu 18.04, you will need to install some extra packages:

```
sudo apt-get install gcc-arm-none-eabi libboost-dev cmake git gcc g++ pkg-config
```

Clone this repository, if you haven't done that already:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

Then simply navigate to the driver directory and build:

```
cd timeswipe/driver
mkdir -p build
cd build
cmake .. -DEMUL=1
make -j$(nproc)
```

Then build the example:

```
cd timeswipe/driver/examples/DataLogging
mkdir -p build
cd build
cmake .. -DEMUL=1
make -j$(nproc) main_static
```
