# TimeSwipe
Firmware and Drivers for PANDA TimeSwipe Boards

# Building firmware and driver

In the "releases" section there are built files for the firmware and the driver, which are ready for flashing. These can be used. Alternatively the procedure of building the firmware or/and the driver from source is described in "Building the firmware" and "Building the driver" in the "docs" folder. 

# Flashing the driver

...

# Flashing the firmware

For flashing a programmer or debugger like the Atmel ICE can be used. More conveniently, since usually the board is plugged onto a Raspberry Pi, the Raspberry Pi can be used as a programmer. How this can be done is described in "Using the Raspberry Pi as programmer with openOCD" in the "docs" folder. <br />

The flashing procedure depends on the programmer which is being used. It usually consists of deleting old data/software in the flash memory of the chip and writing the new software. The flashing procedure is shown for flashing via Raspberry Pi with openOCD as an example:

|                             |                                                   | 
|-----------------------------|  -------------------------------------------------|                       
|1. atsame5 chip-erase	      |   delete old firmware in the flash                |
|2. reset                     |   reset the chip                                  |
|3. halt      		            |   halt the firmware                               |
|4. reset halt                |   reset the chip and keep it in the halted state  |
|5. flash erase_check 0		    |   look for erased/filled sectors                  |
|6. flash write_image xxx.elf	|   flash new firmware (elf-file)                   |

# Product version

The product consists of several modules. Semantic Versioning is used for modules.
Compatibility is defined as follows: for the driver and firmware, compatibility is determined by the major version number.
For example, firmware 1.2.1 is compatible with driver 1.3.5 and 1.4.1 and not compatible with driver 2.0.1
For the compatibility between the submodules of the driver the minor number is also taken into account: for example,
python driver submodule 1.3.9 is compatible with driver 1.3.5 and not compatible with  driver 1.4.1
