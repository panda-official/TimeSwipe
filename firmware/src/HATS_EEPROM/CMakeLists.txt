#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

#HATS_EEPROM sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(HATS_EEPROM_lib)



file(GLOB HATS_EEPROM_SRC "*.cpp")
add_library(HATS_EEPROM_lib STATIC ${HATS_EEPROM_SRC})


target_include_directories(HATS_EEPROM_lib PUBLIC ../Interfaces)
#target_include_directories(SAME54_lib PUBLIC ../ADCDAC)
#target_include_directories(SAME54_lib PUBLIC ../Communication)

target_include_directories(HATS_EEPROM_lib PRIVATE ../../../3rdParty/HATS_EEPROM)


