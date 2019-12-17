/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page Board_page Board
 *
 * This directory contains board's hardware specific components:
 *
 * CADmux - The board's digital multiplexer that implements hardware-dependent realization of
 * setting gain, bridge voltage, enabling ADC measurements and so on.
 * So it provides hardware-dependent realization for some of the nodeControl methods
 *
 * CDac5715sa - a class for controlling board-integrated 4-channel DAC chip.
 *
 * CDACdecor  - a pseudo-decorator class providing CDac-looking methods for overriding DAC behavior depending on CADmux DAC mode
 * (whether to use amplifier outputs for board analog outputs 3/4 or internal SAME54 DACs)
 *
 * CPinButton - an implementation of a button which uses digital pin state as an input signal with a debouncing code.
 *
 * SAMButton - a hardware-dependent realization of the board's button with ability of generation a JSON event from the button state
 *
 * CSamI2CmemHAT - a hardware-dependent realization of CAT2430 EEPROM chip emulation for HAT's outputs
 *
 * CSamI2Cmem8Pin - a hardware-dependent realization of CAT2430 EEPROM chip emulation for external 8-Pin plug outputs
 * 
 */
 
 
