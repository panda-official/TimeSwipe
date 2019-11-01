cmake_minimum_required(VERSION 3.9)

project(main)
SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )

find_package(PkgConfig REQUIRED)
pkg_search_module(timeswipe REQUIRED timeswipe)

include_directories(${timeswipe_INCLUDE_DIRS})
link_directories(${timeswipe_LIBRARY_DIRS})

set(SOURCE_FILES main.cpp)

add_executable(main ${SOURCE_FILES})

target_link_libraries(main ${timeswipe_LIBRARIES})
