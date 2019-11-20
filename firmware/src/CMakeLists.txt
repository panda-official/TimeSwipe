#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)

SET(TARGET_NAME "firmware.elf")

IF(NOT CMAKE_TOOLCHAIN_FILE)
    message(STATUS "User toolchain is not found. Setting default toolchan...")
    SET(CMAKE_TOOLCHAIN_FILE gnuarm_toolchain.cmake)
    include (${CMAKE_TOOLCHAIN_FILE})
ENDIF()

#obtain JSON settings: 05.09.2019
include(ObtainJSONsettings.cmake)


#enabling langs inside:
project(firmware C CXX ASM)

#adding modules:
add_subdirectory(ADCDAC)
add_subdirectory(Procs)
add_subdirectory(Communication)
add_subdirectory(BusinessLogic)
add_subdirectory(JSONstuff)
add_subdirectory(Board)
add_subdirectory(LEDs)
add_subdirectory(CortexMX)
add_subdirectory(SAMe54)


add_executable(${TARGET_NAME}  main.cpp ${PATH_FIRMWARE}/3rdParty/prj_templ/Custom/startup_ARMCM4.S)

target_include_directories(${TARGET_NAME} PUBLIC Interfaces)
target_include_directories(${TARGET_NAME} PUBLIC ADCDAC)
target_include_directories(${TARGET_NAME} PUBLIC JSONstuff)
target_include_directories(${TARGET_NAME} PUBLIC LEDs)
target_include_directories(${TARGET_NAME} PUBLIC Communication)
target_include_directories(${TARGET_NAME} PUBLIC BusinessLogic)
target_include_directories(${TARGET_NAME} PUBLIC Board)
target_include_directories(${TARGET_NAME} PUBLIC SAMe54)

#linking upper layer:
target_link_libraries(${TARGET_NAME} PROCS_lib BLOGIC_lib LEDS_lib JSON_lib)

#linking lower layer:
target_link_libraries(${TARGET_NAME} SAME54_lib CORTEXMX_lib COMM_lib ADCDAC_lib BOARD_lib)

