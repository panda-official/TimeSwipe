#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)

SET(TARGET_NAME "TimeSwipeDriver")

IF(NOT CMAKE_TOOLCHAIN_FILE)
    message(STATUS "User toolchain is not found. Setting default toolchan...")
    SET(CMAKE_TOOLCHAIN_FILE gnua_toolchain.cmake)
    include (${CMAKE_TOOLCHAIN_FILE})
ENDIF()



#enabling langs inside:
project(TimeSwipeDriver C CXX)


add_library(${TARGET_NAME}  STATIC ../3rdParty/BCMsrc/bcm2835.c
                                    RaspberryPi/bcmspi.cpp
                                    )

target_include_directories(${TARGET_NAME} PUBLIC Interfaces)
target_include_directories(${TARGET_NAME} PUBLIC Communication)
target_include_directories(${TARGET_NAME} PUBLIC ../3rdParty/BCMsrc)
target_include_directories(${TARGET_NAME} PUBLIC ./RaspberryPi)


#installing library: 12.09.2019
set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "Interfaces/Serial.h;Interfaces/SPI.h;RaspberryPi/bcmspi.h;Communication/SyncCom.h")

install(TARGETS ${TARGET_NAME}
        LIBRARY
            ARCHIVE DESTINATION lib
        #COMPONENT Libraries
        #NAMELINK_COMPONENT Development
        PUBLIC_HEADER
          DESTINATION include/${TARGET_NAME}
        #  COMPONENT Development
        )
        

