# TimeSwipe
Firmware and Drivers for PANDA TimeSwipe Boards

# Pre-Built Files

The [releases section](https://github.com/panda-official/TimeSwipe/releases) contains pre-built driver and firmware files for various architectures.
You can also compile these yourself.
Instructions can be found in the driver and firmware subdirectories.

# Flashing the firmware

The procedure is explained in the [firmware/README.md](https://github.com/panda-official/TimeSwipe/blob/master/firmware/README.md) section.

# Installing the driver

The procedure is explained in the [driver/README.md](https://github.com/panda-official/TimeSwipe/blob/master/driver/README.md) section.

# Product versioning

The product consists of several modules using semantic versioning.
Compatibility is defined as follows: for the driver and firmware, compatibility is determined by the major version number.
For example, firmware 1.2.1 is compatible with driver 1.3.5 and 1.4.1 and not compatible with driver 2.0.1.
For the compatibility between the submodules of the driver the minor number is also taken into account:
for example, python driver submodule 1.3.9 is compatible with driver 1.3.5 and not compatible with  driver 1.4.1.
