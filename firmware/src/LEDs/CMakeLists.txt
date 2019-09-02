#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)

project(LEDS_lib)


#set(SOURCE_LIB SamCLK.cpp SamSercom.cpp)
#message(STATUS "ASM FLAGS: ${CMAKE_ASM_FLAGS}")

add_library(LEDS_lib STATIC nodeLED.cpp ino_stub_SAME54.cpp ../../3rdParty/Adafruit_NeoPixel/Adafruit_NeoPixel.cpp)

target_compile_definitions(LEDS_lib PRIVATE -D__arm__)
target_compile_definitions(LEDS_lib PRIVATE -DSAM_BRM)
target_compile_definitions(LEDS_lib PRIVATE -D__SAMD51__)
target_compile_definitions(LEDS_lib PRIVATE -D__SAME54P20A__)


target_include_directories(LEDS_lib PRIVATE ../../src/LEDs)
target_include_directories(LEDS_lib PRIVATE ../../3rdParty/Adafruit_NeoPixel)
target_include_directories(LEDS_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)
target_include_directories(LEDS_lib PRIVATE ../../3rdParty/prj_templ/include)





