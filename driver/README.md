# TimeSwipe Driver

The driver is used for the communication between the TimeSwipe board and the Raspberry Pi.
Supported operating systems on the Raspberry Pi are Raspbian Buster (and to an extent, any Debian-based OS), and Arch Linux ARMv8 AArch64.
You can also cross-compile the driver on other host systems.
The `examples` folder contains two example programs which use the driver.


## Installing a Pre-Built Driver

The simplest way to install the TimeSwipe Driver is to download the prebuilt packages.


### Raspbian Buster

If you are running Raspbian Buster (or any other Debian-based OS), simply download the package:

```
wget https://github.com/panda-official/TimeSwipe/releases/download/v0.0.6/timeswipe_0.0.6.armv7l.deb
```

Then simply install with:

```
sudo dpkg -i timeswipe_0.0.6.armv7l.deb
```

You can also uninstall the driver with the command:

```
sudo dpkg -r timeswipe
```


### Arch Linux ARMv8 AArch64

On Arch Linux, similarly download the appropriate package:

```
wget https://github.com/panda-official/TimeSwipe/releases/download/v0.0.6/timeswipe-0.0.6-1-any.pkg.tar.xz
```

And then as `root` install with:

```
pacman -U timeswipe_0.0.6-any.pkg.tar.xz
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

tbd.


### OSX

For corss-compilation on OSX, `ct-ng` and some extra dependencies are needed:

```
brew install crosstool-ng help2man bison
```

Using `Disk Utility` create an APFS (case-sensitive) disk named `xtool-build-env`.


### Build crosstool-ng toolchain
change variables in `.config` file form this directory if disk name is different:

```
CT_WORK_DIR="/Volumes/xtool-build-env/.build"
CT_PREFIX_DIR="/Volumes/xtool-build-env/${CT_TARGET}"
```

change user limit to build:

```
ulimit -n 1024
```

copy file `.config` from this direcory to `/Volumes/xtool-build-env` :

```
cp .config /Volumes/xtool-build-env
```

build toolchain:
```
cd /Volumes/xtool-build-env
ct-ng build
```

After toolchain built successfully change PATH variable:
```
export PATH=/Volumes/xtool-build-env/aarch64-rpi3-linux-gnu/bin/:$PATH
```

### Building driver

Build driver:
```
cd library
mkdir build
cd build
cmake ..
make
```

Build example:
```
cd library/example
mkdir build
cd build
cmake ..
make
```


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

This will gather data for 10 seconds according to the configuration file specified, from the `IEPE` inputs and will save the data in CSV format to the file `temp.txt`.


### Arch Linux ARMv8 AArch64

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

To run the data logging example from the `build` directory, as `root` execute the command:

```
./main --config ../config.json --input IEPE --output temp.txt
```

This will gather data for 10 seconds according to the configuration file specified, from the `IEPE` inputs and will save the data in CSV format to the file `temp.txt`.

