#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)

#obtain JSON settings: 05.09.2019
#include(ObtainJSONsettings.cmake)

project(JSON_lib)
#add_definitions(-D__SAME54P20A__)

#find_package(nlohmann_json 3.2.0 REQUIRED)
#find_package(nlohmann_json REQUIRED)


file(GLOB JSON_SRC "*.cpp")
add_library(JSON_lib STATIC ${JSON_SRC} )

#target_link_libraries(JSON_lib PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(JSON_lib PUBLIC ../Interfaces)
target_include_directories(JSON_lib PUBLIC ../Communication)
#target_include_directories(BLOGIC_lib PUBLIC ../ADCDAC)
#target_include_directories(BLOGIC_lib PUBLIC ../JSONstuff)

#target_compile_definitions(SAME54_lib PRIVATE -D__SAME54P20A__)

#target_include_directories(SAME54_lib PUBLIC ../../../driver/src)
#target_include_directories(SAME54_lib PUBLIC ../ADCDAC)

#target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/include)
#target_include_directories(CORTEXMX_lib PRIVATE ../../3rdParty/prj_templ/CMSIS/Include)


