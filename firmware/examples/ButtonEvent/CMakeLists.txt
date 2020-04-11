#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)

SET(TARGET_NAME "button_event.elf")

SET(BINARY_DIR ../build-ButtonEvent)

IF(NOT CMAKE_TOOLCHAIN_FILE)
    message(STATUS "User toolchain is not found. Setting default toolchan...")
    SET(CMAKE_TOOLCHAIN_FILE ../../src/gnuarm_toolchain.cmake)
    include (${CMAKE_TOOLCHAIN_FILE})
ENDIF()

include_directories(../../../3rdParty/)

SET(CMAKE_CXX_FLAGS "-Wno-psabi")


#fetch firmware dir:
string(REPLACE "examples/ButtonEvent" "" PATH_FIRMWARE ${CMAKE_CURRENT_SOURCE_DIR})



#enabling langs inside:
project(button_event C CXX ASM)

#adding modules:
add_subdirectory(../../src/ADCDAC ${BINARY_DIR}/ADCDAC )
#add_subdirectory(../../src/Procs)
add_subdirectory(../../src/Communication ${BINARY_DIR}/Communication)
#add_subdirectory(../../src/BusinessLogic)
add_subdirectory(../../src/JSONstuff ${BINARY_DIR}/JSONstuff)
add_subdirectory(../../src/Board ${BINARY_DIR}/Board)
add_subdirectory(../../src/LEDs ${BINARY_DIR}/LEDs)
add_subdirectory(../../src/CortexMX ${BINARY_DIR}/CortexMX )
add_subdirectory(../../src/SAMe54 ${BINARY_DIR}/SAMe54)


add_executable(${TARGET_NAME}  main.cpp button_logic.cpp ${PATH_FIRMWARE}/../3rdParty/prj_templ/Custom/startup_ARMCM4.S)

target_include_directories(${TARGET_NAME} PUBLIC ../../src/Interfaces)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/ADCDAC)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/JSONstuff)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/LEDs)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/Communication)
#target_include_directories(${TARGET_NAME} PUBLIC BusinessLogic)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/Board)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/SAMe54)


target_link_libraries(${TARGET_NAME}  LEDS_lib JSON_lib)

#linking lower layer:
target_link_libraries(${TARGET_NAME} SAME54_lib CORTEXMX_lib COMM_lib ADCDAC_lib BOARD_lib)

