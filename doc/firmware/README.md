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

## Base

The following classes represents board's interfaces and hardware specific
components:

  - CMesChannel (aka CIEPEchannel) - provides basic board measurement channel
  functionality and IEPE channel by default;
  - CDMSchannel - provides measurement channel functionality of the DMS board;
  - CShiftRegPin - controls Shift Register of the DMS board to provide pins
  extension;
  - CPGA280 - PGA280 channel amplifier control of the DMS board;
  - CDac5715sa - a class for controlling board-integrated 4-channel DAC chip;
  - CPinButton - an implementation of a button which uses digital pin state as
  an input signal with a debouncing code;
  - SAMButton - a hardware-dependent realization of the board's button with
  ability of generation a JSON event from the button state;
  - CSamI2CmemHAT - a hardware-dependent realization of CAT2430 EEPROM chip
  emulation for HAT's outputs;
  - CSamI2Cmem8Pin - a hardware-dependent realization of CAT2430 EEPROM chip
  emulation for external 8-Pin plug outputs;
  - CDacPWMht - the class implements a PWM which output is controlled by the DAC
  with DMA or timer IRQ support;
  - CPinPWM - the class implements a PWM which output is controlled by the SAM's
  pin with DMA support;
  - CFanControl - the class implements control of fan in PWM mode with several
  fixed speeds;
  - CFanControlSimple - the class implements simple control of fan in ON/OFF mode;
  - CRawBinStorage - provides a mechanism for persistent storing all board
  settings in the SmartEEPROM;
  - CSPIcomm - provides functionality for external communication via SPI with
  integrated flow-control (CSyncSerComFSM).

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

## SAME54 Components Support

### CortexM4 interrupts

#### System Tick Interrupt

Used to generate OS system time returned by os::get_tick_mS().

### Generic Clock Controller

From SAME54 manual, page 152:

```
Depending on the application, peripherals may require specific clock frequencies
to operate correctly. The Generic Clock controller (GCLK) features 12 Generic
Clock Generators that can provide a wide range of clock frequencies.
```

In other words, SAME54 peripherals are not provided with clock frequency by
default, but require a clock generator to be properly tuned and connected. For
this purpose a `CSamCLK` was designed. A `CSamCLK::Factory()` is used to find
free clock generator, reserve it and provide class methods for setup.

### Timers/Counters

`CSamTC` - a realization of SAM's Timer Counter (TC).

### PORT (digital pins IO) control

`CSamPORT` (Port Groups) and `CSamPin` (Port Pins) provides methods for
multiplexing pins to various peripherals like Sercom.

### ADC & DAC

`CSamADCchan` and `CSamADCcntr` are used to control SAME54's ADCs.

`CSamDACcntr` is used to control SAME54's DACs.

`CSamTempSensor` is used to measure SAME54's core temperature.

### QSPI

`CSamQSPI` - is an implementation of Quad SPI Interface. This bus is used to
control MAX15xx board's DAC.

### SERCOM-based serial devices

`SERCOM` - is a SAME54's Serial Communication Interface. Depending on settings
it can be turned into USART, SPI, I2C-master or I2C-slave.

`CSamSercom` provides the basic functionality of SERCOM mainly dealing with
interrupt processing, enabling and connecting corresponding `CSamCLK` (Generic
Clock controller).

`CSamSPIbase` implements basic functionality of SAME54 Sercom SPI.

`CSamI2Cmem` - is an implementation of CAT2430 EEPROM chip emulation
(implementations for concrete pinouts are `CSamI2CmemHAT` and `CSamI2Cmem8Pin`).

`CSamI2CeepromMaster` - is an I2C master for working with a real CAT2430 EEPROM
chip.

### Factory calibration settings

`NVMscpage` - is an interface for reading SAME54 factory calibration settings.

### DMA

`CSamDMABlock`, `CSamDMAChannel` and `CSamDMAC` implements basic DMA functionality.

### Nonvolatile Memory Controller

`CSamNVMCTRL` implements SAM's Nonvolatile Memory Controller with an interface to
SmartEEPROM.

## Control

The basic functionality of the board is provided by the `nodeControl` class.
Since the only one controller object may exist it's designed after the
*singleton* pattern. Features of `nodeControl` includes:

  - adjustment of the board amplifier gain;
  - adjustment of bridge voltage;
  - finding the amplifier offsets;
  - generating a random 32-bit value as a new record stamp.

The board's User Interface (UI) is physically implemented as a button and four
colorized LEDs. The *user menus* are formed by flashing LEDs in different colors.

All the possible views of the board are implemented in a separate View class
`CView` with a view channel for each LED represented by the class `CViewChannel`.

The menu logic is handled by `CNewMenu` class object. The class is designed in
event-driven style and forced by events coming from a button `SAMButton`.

Another feature is a data visualization allowing to display the measured signal
levels by using LEDs when an user is not interacting with the board (default
View mode). This feature is implemented in the `CDataVis` class.

The board's built-in routines and necessary infrastructure for execution are
also landed in this module. Currently, the only finding amplifier offsets
routine is provided by `CADpointSearch` class. This class contains an algorithm
for finding offset for a single channel only. For possibility of finding offsets
for several channels at once there is special infrastructure class `CCalMan`
