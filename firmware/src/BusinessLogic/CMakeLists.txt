#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(BLOGIC_lib)
#add_definitions(-D__SAME54P20A__)



#file(GLOB SAME54_SRC "*.cpp")
add_library(BLOGIC_lib STATIC menu_logic.cpp DataVis.cpp nodeControl.cpp)

target_include_directories(BLOGIC_lib PUBLIC ../Interfaces)
target_include_directories(BLOGIC_lib PUBLIC ../ADCDAC)
target_include_directories(BLOGIC_lib PUBLIC ../JSONstuff)
target_include_directories(BLOGIC_lib PUBLIC ../LEDs)
target_include_directories(BLOGIC_lib PUBLIC ../Board)
target_include_directories(BLOGIC_lib PUBLIC ../Procs)
target_include_directories(BLOGIC_lib PUBLIC ../Communication)

#target_compile_definitions(SAME54_lib PRIVATE -D__SAME54P20A__)

#target_include_directories(SAME54_lib PUBLIC ../../../driver/src)
#target_include_directories(SAME54_lib PUBLIC ../ADCDAC)

#target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/include)
#target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)


