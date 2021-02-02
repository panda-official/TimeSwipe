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

## Communication

The communication with the master device is designed after the *master-slave
model*. *Master* device sends a command request and waiting for a response from
a board. Handling of corresponding communication protocols and commands
dispatching is implemented via Port (basic CStdPort), Stream (basic CFrmStream)
and Command Dispatcher (CCmdDispatcher) concepts.

The Port is responsible for handling specific communication protocol. Being
connected to a serial device object CSerial as event listener it parses the
incoming message to match the communication protocol. When correct message is
detected it forms a protocol-independent request in CCmdCallDescr and send it
to CCmdDispatcher object. Thus several communication protocols can be handled
at once by creating a number of specific Port objects.

The Stream is used for storing call parameters and data in a serialized form and
provide a mechanism for retrieving/storing primitive data types (`int`, `float`,
`std::string`, etc) from/to the stream. The current stream object type determines
a storage format (text, binary, etc).

The Command Dispatcher is used to dispatch command request in protocol-independent
format to corresponding command handler.

Finally, to implement primary communication protocol based on "access point"
concept (see [Communication Protocol](doc/CommunicationProtocol.md)) its necessary
to transform a command request from protocol-independent format to C++ function/class
method call with `get` and `set` signatures like `APtype get()` and `void set(APtype)`
where `APtype` is an access point type. For this purpose two special command handlers
are used: CCmdSGHandler - for binding to class methods, CCmdSGHandlerF - for binding
to functions or static class methods. Thus a communication *access point* can be
created from already existing class methods with appropriate signature by binding
them with CCmdSGHandler for example.

Since normally SPI bus is used for communication a kind of flow-control is also
required. It is realized via CSyncSerComFSM.

## ADCDAC

Implementation of ADC and DAC device management is based on the AD channel concept:
ADC and DAC devices usually contains a number of measurement/controlling units
called *channels*. The common features of a channel described in the `CADchan`
class. The concrete methods for ADC and DAC channel in `CAdc` and `CDac` classes.

Being derived from `CAdc` class ADC channels can be added to corresponding ADC
board object that is responsible for continuous updating channels in a queue.
For example, `CSamADCchan` and `CSamADCcntr` where `CSamADCcntr` implements the
board object. To avoid waiting in queue update and switching between channels
overridden direct measurement function `CAdc::DirectMeasure()` can be called.

An object of class derived from `CDac` can be also added to a board object or
alternatively the control functionality can be implemented directly via
overridden `CDac::DriverSetVal()` since continuous updating and queuing are
usually not required for the DAC channel. The class `CDac5715sa` is an example
of self-controlled channel.
