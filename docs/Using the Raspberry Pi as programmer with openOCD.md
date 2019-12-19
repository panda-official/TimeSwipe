# Using the Raspberry Pi as programmer with openOCD

## Installing openOCD on a DebianOS (like [Ubuntu](https://wiki.ubuntu.com/ARM/RaspberryPi) or [Raspbian](https://www.raspberrypi.org/downloads/raspbian/))

1) Updating package manager
2) Installing dependencies
3) Clone Repository (git://git.code.sf.net/p/openocd/code)
4) Make config for rPi and its GPIO
5) Build
6) Install

```
sudo apt-get update
sudo apt-get install git autoconf libtool make pkg-config libusb-1.0-0 libusb-1.0-0-dev
git clone git://git.code.sf.net/p/openocd/code openocd
cd openocd
./bootstrap
./configure --enable-sysfsgpio --enable-bcm2835gpio
make
sudo make install
```

## Starting openOCD

### Creating a script for openOCD:  

Create a file „PandaOCD.cfg“ with the following content:
   

	\# GPIO17 ---> RST
	\# GPIO27 ---> DCLK
	\# GPIO22 ---> DIO
	\# GND    ---> GND
	
	bindto 0.0.0.0
	
	interface bcm2835gpio
	
	bcm2835gpio_peripheral_base 0x3F000000
	
	\#adapter_khz 1000
	
	\# Transition delay calculation: SPEED_COEFF/khz - SPEED_OFFSET
	\# These depend on system clock, calibrated for stock 900MHz
	\# bcm2835gpio_speed SPEED_COEFF SPEED_OFFSET
	bcm2835gpio_speed_coeffs 146203 36
	
	\# Each of the SWD lines need a gpio number set: swclk swdio
	\# Header pin numbers: GPIO27 GPIO22
	bcm2835gpio_swd_nums 27 22
	
	bcm2835gpio_srst_num 17
	reset_config srst_only srst_push_pull
	
	transport select swd
	
	\#set WORKAREASIZE 0x2000
	source [find target/atsame5x.cfg]
	\#reset_config none
	
	
### Starting openOCD:  

Run `sudo openocd -f PandaOCD.cfg`.

You should see something like:

 adapter speed: 2000 kHz <br />
 cortex_m reset_config sysresetreq <br />
 Info : Listening on port 6666 for tcl connections <br />
 Info : Listening on port 4444 for telnet connections <br />
 Info : BCM2835 GPIO JTAG/SWD bitbang driver <br />
 Info : SWD only mode enabled (specify tck, tms, tdi and tdo gpios to add JTAG mode) <br />
 Info : clock speed 2002 kHz <br />
 Info : SWD DPIDR 0x2ba01477 <br />
 Info : atsame5.cpu: hardware has 6 breakpoints, 4 watchpoints <br />
 Info : atsame5.cpu: external reset detected <br />
 Info : Listening on port 3333 for gdb connections 

### Open a telnet connection:  

In a new terminal window on your computer type `telnet ip_rPi 4444` , ip_rPi: IP-adress of your Raspberry Pi. (E.g.: telnet 10.0.0.1 4444)<br />
Depending on the operating system on the computer it is possible that telnet has to be activated (Not necessary for Ubuntu).

## Flashing with openOCD  

Type openOCD commands in the window with the open telnet connection:

|                              |                                     | 
|----------------------------- |  -----------------------------------|                       
|1. atsame5 chip-erase	       |   delete old software in the flash |
|2. reset init	               |   halt                             |
|3. flash erase_check 0	       |   look if erasing has been completed |
|4. flash write_image xxx.elf  |   flash new software (elf-file)      |
|5. flash erase_check 0	       |   look if software has been flashed

<br />
Some openOCD commands: <br />
<br />

Command                            |  Function
---------------------------------- | ---------------------------------------------
reset init                         |  halt
flash erase_check 0                |  shows the data in the flash
flash erase_sector 0 4 4           |  erases sector 4 on bank 0
atsame5 chip-erase                 |  erases the whole flash, use this command (does not require reset init)
flash write_image xxx.elf          |  flash an elf-file


<br />
<br />
A debugging connection (gdb) can be established just like the telnet connection on port 3333. 

<br />
<br />

### Dependencies

- autoconf :  	https://www.gnu.org/software/autoconf/  
	Autoconf is an extensible package of M4 macros that produce shell 	scripts to automatically configure software source code packages. 	These scripts can adapt the packages to many kinds of UNIX-like 	systems without manual user intervention.

- libtool :
	https://www.gnu.org/software/libtool/ 
	GNU libtool is a generic library support script. Libtool hides the 	complexity of using shared libraries behind a consistent, portable 	interface. 
	
- make :
	https://www.gnu.org/software/make/ 
	GNU Make is a tool which controls the generation of executables 	and other non-source files of a program from the program's source 	files. Make gets its knowledge of how to build your program from a 	file called the makefile, which lists each of the non-source files and 	how to compute it from other files.  
  
 - pkg-config: 
	https://www.freedesktop.org/wiki/Software/pkg-config/  	pkg-config is a helper tool used when compiling applications and 	libraries. It helps you insert the correct compiler options on the 	command line so an application can use gcc -o test test.c `pkg-	config --libs --cflags glib-2.0` for instance, rather than hard-	coding values on where to find glib (or other libraries). It is 	language-agnostic, so it can be used for defining the location of 	documentation tools, for instance. 
  
- libusb-1.0-0 & libusb-1.0-0-dev : 	https://libusb.info/  	libusb is a C library that provides generic access to USB devices. It 	is intended to be used by developers to facilitate the production of 	applications that communicate with USB hardware.
