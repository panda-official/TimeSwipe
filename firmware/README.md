# TimeSwipe Firmware

The TimeSwipe firmware can be flashed onto the TimeSwipe board a programmer or a debugger like the Atmel ICE.
The most convenient variant however is to use a Raspberry Pi with openOCD.
The firmware can either be downloaded from [the releases section](https://github.com/panda-official/TimeSwipe/releases) or can be built on the Raspberry Pi.

## Building the Firmware

For building the firmware, Raspbian Buster (and to an extent, all Debian-based systems) and Arch Linux 64 are supported.

### Building on Raspbian Buster

On a vanilla installation of Raspbian Buster, the following packages have to be installed:

```
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi cmake git 
```

You can then clone this repository, if you haven't done that already:

```
git clone --recursive https://github.com/panda-official/timeswipe.git
```

We then have to build the `JSON` library:

```
cd timeswipe/driver/3rdParty/nlohmann/
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

Then navigate back to the firmware's source directory:

```
cd ../../../../firmware/src
```

We can then build the driver:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

The last step should generate a `firmware.elf` file, which can be flashed to the TimeSwipe board.


### Building on Arch Linux 64

tbd.

## Flashing the Firmware

tbd.

### Using openOCD on a Raspberry Pi

tbd.
