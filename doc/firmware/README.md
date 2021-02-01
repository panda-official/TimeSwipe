# PANDA TimeSwipe Firmware Introduction

## Interfaces

Many real-world hardware devices can be described in terms of interfaces. For
example, there is a communication interface for serial devices like UART or SPI
that provides *send* and *receive* functionality for trasmitting the data. The
basic classes to build any serial device are `ISerial`, `ISerialEvent`, `CSerial`.

`CSPI` is used to add specific SPI bus functionality to a `CSerial` device. In
real hardware devices a data (series of bytes) are normally stored in FIFO
buffer. The buffer is described in `CFIFO` which is also used as the basic data
primitive all over the firmware.

The basic timer and button interfaces are defined in `CTimerEvent` and
`CButtonEvent` classes. The service interface of the operating system is
defined in the `os.h`. The interface for a persistent data storage and objects
serialization interface are described in `CStorage` and `ISerialize` classes.
The generic pin control interface is defined in `CPin` class.
