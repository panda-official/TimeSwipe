#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(CORTEXMX_lib)
#add_definitions(-D__SAME54P20A__)



#file(GLOB SAME54_SRC "*.cpp")
add_library(CORTEXMX_lib STATIC nodeTimeCM4.cpp)

target_compile_definitions(CORTEXMX_lib PRIVATE -D__SAME54P20A__)

#target_compile_definitions(SAME54_lib PRIVATE -D__SAME54P20A__)

#target_include_directories(SAME54_lib PUBLIC ../../../driver/src)
#target_include_directories(SAME54_lib PUBLIC ../ADCDAC)

target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/include)
target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)


