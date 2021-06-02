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

## Versioning

Both TimeSwipe Driver and Firmware follows [semantic versioning 2.0.0][semver].
Compatibility is defined by major version numbers, i.e. driver and firmware are
compatible if theirs major version numbers are equal.

[semver]: https://semver.org/spec/v2.0.0.html
