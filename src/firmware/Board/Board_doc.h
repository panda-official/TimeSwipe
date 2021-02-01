/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page Board_page Board
 *
 * This directory contains board's interfaces and hardware specific components:
 *
 * CMesChannel (aka CIEPEchannel) - provides basic board measurement channel functionality and IEPE channel by default
 *
 * CDMSchannel - provides measurement channel functionality of the DMS board
 *
 * CShiftRegPin - controls Shift Register of the DMS board to provide pins extension
 *
 * CPGA280 - PGA280 channel amplifier control of the DMS board
 *
 * CDac5715sa - a class for controlling board-integrated 4-channel DAC chip.
 *
 * CPinButton - an implementation of a button which uses digital pin state as an input signal with a debouncing code.
 *
 * SAMButton - a hardware-dependent realization of the board's button with ability of generation a JSON event from the button state
 *
 * CSamI2CmemHAT - a hardware-dependent realization of CAT2430 EEPROM chip emulation for HAT's outputs
 *
 * CSamI2Cmem8Pin - a hardware-dependent realization of CAT2430 EEPROM chip emulation for external 8-Pin plug outputs
 *
 * CDacPWMht - the class implements a PWM which output is controlled by the DAC with DMA or timer IRQ support
 *
 * CPinPWM - the class implements a PWM which output is controlled by the SAM's pin with DMA support
 *
 * CFanControl - the class implements control of fan in PWM mode with several fixed speeds
 *
 * CFanControlSimple - the class implements simple control of fan in ON/OFF mode
 * 
 * CRawBinStorage - provides a mechanism for persistent storing all board settings in the SmartEEPROM
 *
 * CSPIcomm - provides functionality for external communication via SPI with integrated flow-control (CSyncSerComFSM)
 *
 */
 
 
