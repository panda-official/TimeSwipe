#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(COMM_lib)
#add_definitions(-D__SAME54P20A__)



file(GLOB COMM_SRC "*.cpp")
add_library(COMM_lib STATIC ${COMM_SRC} )

target_include_directories(COMM_lib PUBLIC ../Interfaces)
#target_include_directories(BLOGIC_lib PUBLIC ../ADCDAC)
#target_include_directories(BLOGIC_lib PUBLIC ../JSONstuff)

#target_compile_definitions(SAME54_lib PRIVATE -D__SAME54P20A__)

#target_include_directories(SAME54_lib PUBLIC ../../../driver/src)
#target_include_directories(SAME54_lib PUBLIC ../ADCDAC)

#target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/include)
#target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)


