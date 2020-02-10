#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)

SET(TARGET_NAME "rPIspi")

project(SPIterminal C CXX)

SET(CMAKE_C_FLAGS "-lpthread -O3 -latomic")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -O3 -Wno-psabi -latomic")

add_executable(${TARGET_NAME} main.cpp console.cpp)

target_include_directories(${TARGET_NAME} PUBLIC /usr/local/include/TimeSwipeDriver)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/Interfaces)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/Communication)
target_include_directories(${TARGET_NAME} PUBLIC ../../src/RaspberryPi)

target_link_libraries(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/../../build/libtimeswipe.a ${timeswipe_LIBRARIES})


