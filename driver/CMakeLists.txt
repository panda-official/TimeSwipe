cmake_minimum_required(VERSION 3.9)

add_definitions(-O3)
SET(CMAKE_C_FLAGS "-lpthread -O3")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -O3")

file (STRINGS "version" BUILD_NUMBER)
project(timeswipe VERSION ${BUILD_NUMBER} DESCRIPTION "timeswipe driver")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
SET(CMAKE_C_COMPILER gcc )
SET(CMAKE_CXX_COMPILER g++ )
else ()
SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
endif ()
SET(CMAKE_INSTALL_LIBDIR lib)

include(GNUInstallDirs)

SET(SRC
    src/timeswipe.cpp
	3rdParty/BCMsrc/bcm2835.c
	src/Communication/SyncCom.cpp
	src/RaspberryPi/bcmspi.cpp
)

add_library(timeswipe SHARED ${SRC})
add_library(timeswipeStatic STATIC ${SRC})
set_target_properties(timeswipeStatic PROPERTIES OUTPUT_NAME "timeswipe")

set_target_properties(timeswipe PROPERTIES PUBLIC_HEADER include/timeswipe.hpp)
set_target_properties(timeswipeStatic PROPERTIES PUBLIC_HEADER include/timeswipe.hpp)

set_target_properties(timeswipe PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(timeswipeStatic PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(timeswipe PROPERTIES SOVERSION 1)

SET(timeswipe_include_dirs src src/RaspberryPi src/Interfaces src/Communication 3rdParty/BCMsrc)
target_include_directories(timeswipe PRIVATE ${timeswipe_include_dirs})
target_include_directories(timeswipeStatic PRIVATE ${timeswipe_include_dirs})

target_include_directories(timeswipe PRIVATE include)
target_include_directories(timeswipeStatic PRIVATE include)

set_target_properties(timeswipe PROPERTIES CXX_STANDARD 17)
set_target_properties(timeswipeStatic PROPERTIES CXX_STANDARD 17)


install(TARGETS timeswipe
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(contrib/timeswipe.pc.in timeswipe.pc @ONLY)
configure_file(contrib/DEBIAN/control.in DEBIAN/control @ONLY)
configure_file(contrib/arch/PKGBUILD.in PKGBUILD @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/timeswipe.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

