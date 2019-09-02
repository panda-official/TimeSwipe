cmake_minimum_required(VERSION 3.1)

IF(NOT CMAKE_TOOLCHAIN_FILE)
    message(STATUS "User toolchain is not found. Setting default toolchan...")
    SET(CMAKE_TOOLCHAIN_FILE gnuarm_toolchain.cmake)
    include (${CMAKE_TOOLCHAIN_FILE})
ENDIF()

#find_package(nlohmann_json REQUIRED)
IF(NOT JSON_INCLUDE_DIR)
LIST(APPEND FIND_JSON json.hpp)
FIND_PATH(JSON_INCLUDE_DIR ${FIND_JSON}
    PATH_SUFFIXES include
    HINTS json/include/
   CMAKE_FIND_ROOT_PATH_BOTH
)
ENDIF()

message(STATUS "Found a JSON path: ${JSON_INCLUDE_DIR}")
include_directories(${JSON_INCLUDE_DIR})


#enabling langs inside:
project(firmware C CXX ASM)

#adding modules:
add_subdirectory(ADCDAC)
add_subdirectory(Procs)
add_subdirectory(Communication)
add_subdirectory(BusinessLogic)
add_subdirectory(JSONstuff)
add_subdirectory(Board)
add_subdirectory(LEDs)
add_subdirectory(CortexMX)
add_subdirectory(SAMe54)


add_executable(firmware  main.cpp ${PATH_FIRMWARE}/3rdParty/prj_templ/Custom/startup_ARMCM4.S)

target_include_directories(firmware PUBLIC Interfaces)
target_include_directories(firmware PUBLIC ADCDAC)
target_include_directories(firmware PUBLIC JSONstuff)
target_include_directories(firmware PUBLIC LEDs)
target_include_directories(firmware PUBLIC Communication)
target_include_directories(firmware PUBLIC BusinessLogic)
target_include_directories(firmware PUBLIC Board)
target_include_directories(firmware PUBLIC SAMe54)

#linking upper layer:
target_link_libraries(firmware PROCS_lib BLOGIC_lib LEDS_lib JSON_lib)

#linking lower layer:
target_link_libraries(firmware SAME54_lib CORTEXMX_lib COMM_lib ADCDAC_lib BOARD_lib)

