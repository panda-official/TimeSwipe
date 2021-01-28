![](https://github.com/panda-official/TimeSwipe/workflows/Build%20Linux%20Packages/badge.svg)

# PANDA TimeSwipe Driver and Firmware

The [releases section](https://github.com/panda-official/TimeSwipe/releases)
contains pre-built driver for both ARM32 and ARM64 architectures.

## Documentation

[Driver API](https://panda-official.github.io/TimeSwipe/index.html)

### General documentation

- [Communication protocol](docs/CommunicationProtocol.md)
- [Event system](docs/EventSystem.md)

### Building and installing the software

- [Software dependencies](docs/SoftwareDependencies.md)
- [Building and installing the driver](docs/driver.md)
- [Building and flashing the firmware](docs/firmware.md)

## Product versioning

The product consists of several modules using semantic versioning. Compatibility
is defined by major version number, i.e. driver and firmware are compatible if
their major version numbers are equal. Compatibility between the submodules of
the driver is define by minor version number.

For example, the firmware 1.2.1 is compatible with the driver 1.3.5, but not
compatible with the driver 2.0.1; the Python driver submodule 1.3.9 is compatible
with the driver 1.3.5, but not compatible with the driver 1.4.1.
