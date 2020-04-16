# TimeSwipe Firmware

The TimeSwipe firmware can be flashed onto the TimeSwipe board a programmer or a debugger like the Atmel ICE.
The most convenient variant however is to use a Raspberry Pi with openOCD.
The firmware can either be downloaded from [the releases section](https://github.com/panda-official/TimeSwipe/releases) or can be built on the Raspberry Pi.


## Building the Firmware

For building the firmware, Raspbian Buster (and to an extent, all Debian-based systems) and Arch Linux ARMv8 AArch64 are supported.


### Building on Raspbian Buster

On a vanilla installation of Raspbian Buster, the following packages have to be installed:

```
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi cmake git 
```

You can then clone this repository, if you haven't done that already:

```
git clone https://github.com/panda-official/timeswipe.git
```

Then navigate to the firmware's source directory:

```
cd firmware/src
```

We can then build the driver:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

The last step should generate a `firmware.elf` file, which can be flashed to the TimeSwipe board.


### Building on Arch Linux ARMv8 AArch64

tbd.


## Flashing the Firmware with openOCD on a Raspberry Pi

The easiest way to flash the firmware, is to use a Raspberry Pi.


### Flashing with Raspbian Buster

To flash the firmware directly from the Raspberry Pi, we need to install `openOCD`:

```
sudo apt-get install git autoconf libtool make pkg-config libusb-1.0-0 libusb-1.0-0-dev telnet
git clone git://git.code.sf.net/p/openocd/code openocd
cd openocd
./bootstrap
./configure --enable-sysfsgpio --enable-bcm2835gpio
make -j$(nproc)
sudo make install
cd ..
```

After installation, run openOCD with the `PandaOCD.cfg` file located in the firmware directory:

```
sudo openocd -f timeswipe/firmware/PandaOCD.cfg
```

You should see something like:

```
adapter speed: 2000 kHz
cortex_m reset_config sysresetreq
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : BCM2835 GPIO JTAG/SWD bitbang driver
Info : SWD only mode enabled (specify tck, tms, tdi and tdo gpios to add JTAG mode)
Info : clock speed 2002 kHz
Info : SWD DPIDR 0x2ba01477
Info : atsame5.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : atsame5.cpu: external reset detected
Info : Listening on port 3333 for gdb connections
```

OpenOCD is waiting for input.
For this, open a `telnet` connection to the Pi, by switching to a second console (press `Ctrl + Alt + F2`) and entering the command:

```
telnet localhost 4444
```

To reset the chip and keep it in the `halt`state, use:

```
reset halt
```

To prepare the chip for flash erase/write operations, use:

```
reset init
```

To delete any old firmware in the flash memory of the TimeSwipe board, enter the command:

```
atsame5 chip-erase
```

To verify that the previous step worked, use:

``` 
flash erase_check 0
```

You should see output along the lines of `Bank is erased`.
To write a new firmware, enter:

```
flash write_image timeswipe/firmware/src/build/firmware.elf
```

This assumes you are running openOCD from the home directory and the firmware has been built in the default path.
Change accordingly.
Verify the write process by again entering:

```
flash erase_check 0
```

You should see several outputs of `... not erased`.
The exact count will depend on the size of the firmware image, and thus the amount of memory it occupies in the flash memory.
This indicates a successful write process.

After the firmware has been written, it can be started by:

```
reset
```


Type `exit` to disconnect the telnet session and again `exit` to log out of the second console.
Switch to the first console with `Ctrl + Alt + F1` and exit openOCD with `Ctrl + c`.
Shutdown the Raspberry Pi and the TimeSwipe board and power on again to start using the firmware.


### Flashing with Arch Linux ARMv8 AArch64

tbd.

