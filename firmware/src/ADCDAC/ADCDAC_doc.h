/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page ADCDAC_page ADCDAC
 *
 *  Implementation of ADC and DAC device management is based on the AD channel concept:
 *  ADC and DAC devices usually contain a number of measurement/controlling units called channels.
 *  The common features of a channel described in the CADchan class. The concrete methods for ADC and DAC channel in
 *  CAdc and CDac classes.
 *
 *  Being derived from CAdc class ADC channels can be added to corresponding ADC board object that is responsible for continuous updating
 *  channels in a queue. For example - CSamADCchan and CSamADCcntr where CSamADCcntr implements the board object.
 *  To avoid waiting in updation queue and switching between channels overridden direct measurement function of the CAdc can be called:
 *  CAdc#DirectMeasure()
 *
 *  CDac derived object can be also added to a board object or alternatively the control functionality can be implemented directly via
 *  overridden CDac#DriverSetVal() since continuous updating and queuing usually not required for the DAC channel. The example of self-controlled
 *  channel can be found in CDac5715sa class
 *
 */
 
 
