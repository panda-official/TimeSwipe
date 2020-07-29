#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)

SET(TARGET_NAME "firmware.elf")

option(DMS "Whether build firmware for the DMS board" ON)

IF(NOT CMAKE_TOOLCHAIN_FILE)
    message(STATUS "User toolchain is not found. Setting default toolchain...")
    SET(CMAKE_TOOLCHAIN_FILE gnuarm_toolchain.cmake)
    include (${CMAKE_TOOLCHAIN_FILE})
ENDIF()

include_directories(../../3rdParty/)
include_directories(BusinessLogic)

SET(CMAKE_CXX_FLAGS "-Wno-psabi")

#enabling langs inside:
project(firmware C CXX ASM)

#override JSON formatting:
add_compile_definitions(NLOHMANN_JSON_CUSTOM_STDIO_FRM)

IF(DMS)
    add_compile_definitions(DMS_BOARD)
ENDIF()

#adding modules:
add_subdirectory(ADCDAC)
add_subdirectory(Procs)
add_subdirectory(Communication)
add_subdirectory(BusinessLogic)
add_subdirectory(JSONstuff)
add_subdirectory(HATS_EEPROM)
add_subdirectory(Board)
add_subdirectory(LEDs)
add_subdirectory(CortexMX)
add_subdirectory(SAMe54)


add_executable(${TARGET_NAME}  main.cpp ${PATH_FIRMWARE}/../3rdParty/prj_templ/Custom/startup_ARMCM4.S)

target_include_directories(${TARGET_NAME} PUBLIC Interfaces)
target_include_directories(${TARGET_NAME} PUBLIC ADCDAC)
target_include_directories(${TARGET_NAME} PUBLIC JSONstuff)
target_include_directories(${TARGET_NAME} PUBLIC LEDs)
target_include_directories(${TARGET_NAME} PUBLIC Communication)
target_include_directories(${TARGET_NAME} PUBLIC BusinessLogic)
target_include_directories(${TARGET_NAME} PUBLIC Board)
target_include_directories(${TARGET_NAME} PUBLIC SAMe54)
target_include_directories(${TARGET_NAME} PUBLIC HATS_EEPROM)

#linking upper layer:
target_link_libraries(${TARGET_NAME}  BLOGIC_lib PROCS_lib LEDS_lib JSON_lib HATS_EEPROM_lib)

#linking lower layer:
target_link_libraries(${TARGET_NAME} BOARD_lib COMM_lib ADCDAC_lib)

target_link_libraries(${TARGET_NAME} SAME54_lib CORTEXMX_lib)

target_link_libraries(${TARGET_NAME} BOARD_lib)


