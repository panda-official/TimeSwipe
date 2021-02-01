/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page SAMe54_page SAMe54
 *
 * This directory contains a class library for controlling board's CPU (SAME54) hardware components.
 *
 * Currently the following parts are implemented:
 *
 * <H4> Generic Clock Controller </H4>
 *
 * <I> "Depending on the application, peripherals may require specific clock frequencies to operate correctly. The
 *   Generic Clock controller (GCLK) features 12 Generic Clock Generators that can provide a wide
 *   range of clock frequencies." </I> SAME54 manual, page 152.
 *
 *  In other words, SAME54 peripherals are not provided with clock frequency by default,
 *  but require a clock generator to be properly tuned and connected. For this purpose a CSamCLK was designed.
 *  A CSamCLK::Factory() used to find free clock generator, reserve it and provide class methods for setup.
 *
 * <H4> Timers/Counters </H4>
 *
 *  CSamTC - a realization os SAM's TC (Timer-Counter)
 *
 * <H4> PORT(digital pins IO) control </H4>
 *
 *  CSamPORT, CSamPin - defines Port Groups and Port Pins of the SAME54, single pin control class and methods for multiplexing pins to various peripherals like Sercom
 *
 * <H4> ADC & DAC </H4>
 *
 *  CSamADCchan and CSamADCcntr are used to control SAME54's ADCs.
 *  CSamDACcntr is used to control SAME54's DACs
 *  CSamTempSensor is used to measure SAME54's core temperature
 *
 * <H4> QSPI </H4>
 *
 * CSamQSPI - an implementation of  Quad SPI Interface. This bus is used to control MAX15xx board's DAC.
 *
 * <H4> SERCOM-based serial devices </H4>
 *
 * SERCOM is a SAME54's Serial Communication Interface.
 * Depending on settings it can be turned into USART, SPI, I2C-master or I2C-slave.
 *
 * CSamSercom provides the basic functionality of SERCOM mainly dealing with interrupt processing, enabling and connecting corresponding
 * CSamCLK (Generic Clock controller)
 *
 * CSamSPIbase - the class implements basic functionality of SAME54 Sercom SPI
 *
 * CSamI2Cmem - a realization of CAT2430 EEPROM chip emulation (realizations for concrete pinouts are CSamI2CmemHAT & CSamI2Cmem8Pin)
 *
 * CSamI2CeepromMaster - an I2C master for working with a real CAT2430 EEPROM chip
 *
 * <H4> Factory calibration settings </H4>
 *
 * NVMscpage - an interface for reading SAME54 factory calibration settings
 *
 * <H4> DMA </H4>
 *
 * CSamDMABlock, CSamDMAChannel, CSamDMAC - implement basic DMA functionality
 * 
 * <H4> Nonvolatile Memory Controller </H4>
 *
 * CSamNVMCTRL - implements SAM's Nonvolatile Memory Controller with an interface to SmartEEPROM
 */
 
 
