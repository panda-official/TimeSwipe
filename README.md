# TimeSwipe

Firmware and Drivers for PANDA TimeSwipe Boards

# Quick Start Guide

Welcome to the TimeSwipe Board embedded software developer quick start guide!

This document describes the hardware & software setup required to start software development process for the board,
provides a brief information about the board, tools
respective I/O pins, and connectors(?).
For detailed information refer to the respective
user guides available for download from the following location (...)


## Hardware setup

Provide the board with a power by connecting a power plug, then connect a LAN cable to the board. The board should appear in your local area network.


## Software setup

We recommend to install the following software on your host machine:

1) CMAke building system - https://cmake.org/download/

2) GNUARM cross-toolchain required to build the firmware https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

3) A cross-toolchain for Raspberry required to build corresponding driver and library (?)

4) Clone this repository on your host machine.

After a proper software installation run a "cmake" command to build all of the necessary software packages.

Being built packages are ready for an installation. Run "cmake install" command to install a  end-production release package.
If you'd like to customize software by some changes or just investigate how it is working there is an ability of installing debug package by "cmake install_D".

The debug session can be handled by the IDE of your choice. The examples of using most popular IDE's for debugging are given in the docs:

Eclipse IDE: (...)
QTcreator IDE: (...)

Also there are a number of useful tools/utilities for debugging and service purposes.

## Python friendly
(...)


