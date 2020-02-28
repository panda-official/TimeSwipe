#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team


cmake_minimum_required(VERSION 3.9)

project(rPIspi)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
SET(CMAKE_C_COMPILER gcc )
SET(CMAKE_CXX_COMPILER g++ )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
SET(CMAKE_C_COMPILER aarch64-rpi3-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-rpi3-linux-gnu-g++ )
SET(CMAKE_AR aarch64-rpi3-linux-gnu-ar )
SET(CMAKE_C_FLAGS "-lpthread -fPIC")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -fPIC")
set(CMAKE_CXX_LINK_FLAGS "")
set(CMAKE_C_LINK_FLAGS "")
set(CMAKE_OSX_SYSROOT "")
elseif (${ARM32})
SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++ )
else ()
SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
endif ()
SET(CMAKE_C_FLAGS "-lpthread -O3")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -O3")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#include_directories(../../include ../../3rdParty ../../3rdParty/nlohmann/include)
else ()
find_package(PkgConfig REQUIRED)
pkg_search_module(timeswipe REQUIRED timeswipe)

#include_directories(${timeswipe_INCLUDE_DIRS} ../../3rdParty ../../3rdParty/nlohmann/include)
link_directories(${timeswipe_LIBRARY_DIRS})
endif ()

#additional includes:
include_directories(../../src/RaspberryPi ../../../firmware/src/Interfaces  ../../../firmware/src/Communication)



set(SOURCE_FILES main.cpp console.cpp)

add_executable(rPIspi ${SOURCE_FILES})
add_executable(rPIspi_static ${SOURCE_FILES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
target_link_libraries(rPIspi ${CMAKE_SOURCE_DIR}/../../build/libtimeswipe.a)
set(CMAKE_LD_FLAGS "")
set(CMAKE_SHARED_LINKER_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")
target_compile_options(rPIspi PRIVATE -fexceptions)
else ()
target_link_libraries(rPIspi_static ${CMAKE_SOURCE_DIR}/../../build/libtimeswipe.a ${timeswipe_LIBRARIES})
target_link_libraries(rPIspi ${timeswipe_LIBRARIES})
endif()
