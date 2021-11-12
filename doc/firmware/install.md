# PANDA TimeSwipe Firmware Flashing Guide

The TimeSwipe firmware can be flashed onto the TimeSwipe board a programmer or
a debugger like the Atmel ICE. The most convenient way however is to use
Raspberry Pi with OpenOCD. The firmware can either be downloaded from
[the releases section](https://github.com/panda-official/TimeSwipe/releases) or
can be built on the Raspberry Pi from the sources.

## Caveats

There are two generation of boards exists at the moment: *DMS* (actual) and
*IEPE* (legacy), which are not binary compatible to each other! An improper
firmware can be flashed but simply will not work! By default, when building
from source, the firmware for *DMS* boards is built. In order to build for
*IEPE* board the command line option `-DPANDA_TIMESWIPE_FIRMWARE_DMS=Off` should
be passed to CMake (please see the examples below).

## Building the Firmware

For building the firmware, Raspbian Buster (and to an extent, all Debian-based
systems) and Arch Linux ARMv8 AArch64 are supported.

### Building on Raspbian Buster

On a vanilla installation of Raspbian Buster, the following packages have to be
installed:

```
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi cmake git
```

You can then clone this repository, if you haven't done that already:

```
git clone https://github.com/panda-official/timeswipe.git
```

Then navigate to the project directory:

```
cd timeswipe
```

We can then build the firmware.

```
mkdir -p build_fw
cd build_fw
cmake .. -DPANDA_TIMESWIPE_FIRMWARE=On
make -j$(nproc)
cd ../..
```

The last step should generate a `normal_firmware.elf` file, which can be flashed to
the TimeSwipe board.

## Flashing the Firmware with OpenOCD on a Raspberry Pi

The easiest way to flash the firmware is to use Raspberry Pi.

### Flashing with Raspbian Buster

To flash the firmware directly from Raspberry Pi, we need to install `OpenOCD`:

```
sudo apt install git autoconf libtool make pkg-config libusb-1.0-0 libusb-1.0-0-dev telnet libgpiod-dev
git clone --recursive --branch v0.11.0 git://git.code.sf.net/p/openocd/code openocd
cd openocd
./bootstrap
./configure --enable-linuxgpiod
make -j$(nproc)
sudo make install
cd ..
```

After installation, run OpenOCD with the config file:

```
sudo openocd -f timeswipe/tool/openocd_rpi4_.cfg
```

You should see something like:

```
Open On-Chip Debugger 0.11.0-dirty (2021-10-19-11:03)
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
srst_only separate srst_gates_jtag srst_push_pull connect_deassert_srst

Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : Linux GPIOD JTAG/SWD bitbang driver
Info : This adapter doesn't support configurable speed
Info : SWD DPIDR 0x2ba01477
Info : atsame54p20a.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : starting gdb server for atsame54p20a.cpu on 3333
Info : Listening on port 3333 for gdb connections
```

OpenOCD is waiting for input.
For this, open a `telnet` connection to the Pi, by switching to a second console
(press `Ctrl + Alt + F2`) and entering the command:

```
telnet localhost 4444
```

To flash the newly built firmware, type:

```
program normal_firmware.elf verify reset exit
```

openocd will now erase the device, flash the new firmware, verifies the contents of the flash and finally reset the device.
If no error appears, the firmware was flashed successfully and you should see the LEDs on the timeswipe flash twice as confirmation.

Type `exit` to log out of the second console. Switch to the first console with `Ctrl + Alt + F1` and exit
OpenOCD with `Ctrl + c`. Shutdown the Raspberry Pi and the TimeSwipe board and power on again to start using the new firmware.
