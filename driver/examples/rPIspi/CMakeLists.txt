#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)

SET(TARGET_NAME "rPIspi")

IF(NOT CMAKE_TOOLCHAIN_FILE)
    message(STATUS "User toolchain is not found. Setting default toolchan...")
    SET(CMAKE_TOOLCHAIN_FILE ../../src/gnua_toolchain.cmake)    #???
    include (${CMAKE_TOOLCHAIN_FILE})
ENDIF()


project(SPIterminal C CXX)


add_executable(${TARGET_NAME} main.cpp console.cpp)


target_include_directories(${TARGET_NAME} PUBLIC /usr/local/include/TimeSwipeDriver)
#target_include_directories(${TARGET_NAME} PUBLIC /usr/local/lib)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/Interfaces)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/Communication)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/RaspberryPi)


find_library(LIB_PATH TimeSwipeDriver HINTS /usr/local/lib) #thats the right way!!!
message(STATUS "Found lib: ${LIB_PATH}" )

target_link_libraries(${TARGET_NAME} PUBLIC ${LIB_PATH})


