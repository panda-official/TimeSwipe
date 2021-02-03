![](https://github.com/panda-official/TimeSwipe/workflows/Build%20Linux%20Packages/badge.svg)

# PANDA TimeSwipe Driver and Firmware

The [releases section](https://github.com/panda-official/TimeSwipe/releases)
contains pre-built driver for both ARM32 and ARM64 architectures.

## Documentation

[Driver API](https://panda-official.github.io/TimeSwipe/index.html)

### General documentation

- [Communication protocol](doc/CommunicationProtocol.md)
- [Event system](doc/EventSystem.md)

### Building and installing the software

- [Software dependencies](doc/SoftwareDependencies.md)
- [Building and installing the driver](doc/driver/install.md)
- [Building and flashing the firmware](doc/firmware/install.md)

## Product versioning

The product consists of several modules using semantic versioning. Compatibility
is defined by major version numbers, i.e. driver and firmware are compatible if
their major version numbers are equal. Compatibility between submodules of the
driver is defined by minor version numbers.

For example, the firmware 1.2.1 is compatible with the driver 1.3.5, but not
compatible with the driver 2.0.1; the Python driver submodule 1.3.9 is
compatible with the driver 1.3.5, but not compatible with the driver 1.4.1.
