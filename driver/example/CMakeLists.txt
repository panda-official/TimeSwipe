cmake_minimum_required(VERSION 3.9)

project(main)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
SET(CMAKE_C_COMPILER gcc )
SET(CMAKE_CXX_COMPILER g++ )
else ()
SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
endif ()
SET(CMAKE_C_FLAGS "-lpthread -O3")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -O3")

find_package(PkgConfig REQUIRED)
pkg_search_module(timeswipe REQUIRED timeswipe)

include_directories(${timeswipe_INCLUDE_DIRS} ../3rdParty)
link_directories(${timeswipe_LIBRARY_DIRS})

set(SOURCE_FILES main.cpp)

add_executable(main ${SOURCE_FILES})

target_link_libraries(main ${timeswipe_LIBRARIES})
