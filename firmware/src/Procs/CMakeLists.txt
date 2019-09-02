#PROCS sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(PROCS_lib)
#add_definitions(-D__SAME54P20A__)


file(GLOB PROCS_SRC "*.cpp")
add_library(PROCS_lib STATIC ${PROCS_SRC})

#target_include_directories(SAME54_lib PUBLIC ../../../driver/src)
target_include_directories(PROCS_lib PUBLIC ../Interfaces)
target_include_directories(PROCS_lib PUBLIC ../ADCDAC)
target_include_directories(PROCS_lib PUBLIC ../JSONstuff)
target_include_directories(PROCS_lib PUBLIC ../LEDs)
target_include_directories(PROCS_lib PUBLIC ../Communication)
target_include_directories(PROCS_lib PUBLIC ../BusinessLogic)
target_include_directories(PROCS_lib PUBLIC ../Board)
target_include_directories(PROCS_lib PUBLIC .)

#target_include_directories(SAME54_lib PRIVATE ../../3rdParty/prj_templ/include)
#target_include_directories(SAME54_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)


