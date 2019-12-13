/*!
 * 
 * \file
 * \brief this file is used only to generate the documentation
 * 
 * \mainpage TimeSwipe Firmware Manual
 *
 * \section intro_sec Introduction
 *
 *  This manual describes the source code structure of the firmware, basic modules and concepts used,
 *  classes hierarchy and function calls graphs.
 *
 * \section src_struct_sec Source code structure
 *
 *  The source code is divided into several logical parts and placed in the corresponding directories
 *
 *  For brief view on the firmware structure please focus on the following components for begining:
 *
 *  main() - the current firmware assemblage point. Here all neccesary firmware objects and modules are created at run-time,
 *  corresponding bindings and links are established between them and the main firmware  loop starts running ("superloop mode")
 *
 *  \ref Interfaces_page - the basic inerfaces, data primitives and classes definitions. Most of firmware classes are based on this data types
 *
 *  To see how analog part of board is managed please refer to \ref ADCDAC_page
 *
 *  A communication with external master device and implementation of communication protocols is another essential part of the firmware.
 *  it can be found at \ref Communication_page
 *
 * The list of all firmware directories is shown below:
 *
 *
 * \ref ADCDAC_page - Analog to Digital Converters and Digital to Analog Converters
 *
 * \ref Board_page  - TimeSwipe board specific components
 *
 * \ref BusinessLogic_page - Business Logic parts that determine behaviour of the board UI components - menues, LEDs and so on
 *
 * \ref Communication_page - The components used for external communication
 *
 * \ref CortexMX_page - CortexMX core specific components
 *
 * \ref Interfaces_page - Basic abstract interfaces
 *
 * \ref JSONstuff_page - JSON support components
 *
 * \ref LEDs_page  - A module for controlling board LEDs
 *
 * \ref Procs_page - Implementation of several routines like finding offsets routine for example
 *
 * \ref SAMe54_page - A class library for controlling SAME54 chip hardware components
 */
