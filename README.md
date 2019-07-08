# TimeSwipe
Firmware and Drivers for PANDA TimeSwipe Boards

# Using the Raspberry Pi as programmer with openOCD

## Installing openOCD 

1) Updating:  <sudo apt-get update>
2) Installing dependencies:  <sudo apt-get install git autoconf libtool make pkg-config libusb-1.0-0 libusb-1.0-0-dev>
3) git clone:  make a directory „openocd“, go to it and git clone there: „https://sourceforge.net/p/openocd/code/“ (http://openocd.org)
4) make config for rPi and its GPIO:  go to the folder openocd-code and type <./bootstrap>, then <./configure --enable-sysfsgpio --enable-bcm2835gpio>
5) build:  <make>
6) install:  <sudo make install>


## Starting openOCD

1) script for openOCD:  put the file „PandaOCD.cfg“ in home/pi
2) start openOCD:  in home/pi type <sudo openocd -f PandaOCD.cfg>

You should see something like:

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

1) telnet connection:  in a new terminal window on your computer type <telnet ip_rPi 4444> , ip_rPi: IP-adress of your Raspberry Pi. (E.g.: telnet 10.0.0.1 4444)
Depending on the operating system on your computer it is possible that telnet has to be activated. Not necessary for Ubuntu.

2) Typing openOCD commands: in the window with the telnet connection it is possible now to type openOCD commands.  For example: 
	•	reset_init : halting
	•	flash erase_check 0 : looking on the flash
	•	flash erase_sector 0 4 4 : erases sector 4 on bank 0
	•	atsame5_chip-erase : the former command only erases one sector. To erase the whole flash, use this command (the only command in this list which does not require reset_init)
	•	flash write_image xxx.elf : flash an elf-file

A debugging connection (gdb) can be established just like the telnet connection on port 3333. 


## Dependencies

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
