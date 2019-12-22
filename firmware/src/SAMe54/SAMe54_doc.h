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
 * <H4> ADC & DAC </H4>
 *
 *  CSamADCchan and CSamADCcntr are used to control SAME54's ADCs.
 *  CSamDACcntr is used to control SAME54's DACs
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
 * CSamSPI is a basic class providing functionality for external communication via SPI with integrated flow-control (CSyncSerComFSM)
 *
 * CSamSPIsc2 - a concrete realizaion of CSamSPI for SERCOM2 and its pinout
 *
 * CSamSPIsc7 - a concrete realizaion of CSamSPI for SERCOM7 and its pinout
 *
 * CSamI2Cmem - a realization of CAT2430 EEPROM chip emulation (realizations for concrete pinouts are CSamI2CmemHAT & CSamI2Cmem8Pin)
 *
 * CSamI2CeepromMaster - an I2C master for working with a real CAT2430 EEPROM chip
 *
 * <H4> Factory calibration settings </H4>
 *
 * NVMscpage - an interface for reading SAME54 factory calibration settings
 * 
 */
 
 
