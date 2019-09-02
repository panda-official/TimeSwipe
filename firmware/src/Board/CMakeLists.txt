#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(BOARD_lib)



file(GLOB BOARD_SRC "*.cpp")
add_library(BOARD_lib STATIC ${BOARD_SRC})

target_compile_definitions(BOARD_lib PRIVATE -D__SAME54P20A__)

#target_include_directories(BOARD_lib PUBLIC ../../../driver/src)
target_include_directories(BOARD_lib PUBLIC ../Interfaces)
target_include_directories(BOARD_lib PUBLIC ../ADCDAC)
target_include_directories(BOARD_lib PUBLIC ../JSONstuff)
target_include_directories(BOARD_lib PUBLIC ../Communication)

target_include_directories(BOARD_lib PRIVATE ../../3rdParty/prj_templ/include)
target_include_directories(BOARD_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)


