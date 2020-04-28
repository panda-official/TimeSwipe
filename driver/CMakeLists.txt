cmake_minimum_required(VERSION 3.9)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-O3)
SET(CMAKE_C_FLAGS "-lpthread -O3 -Wall")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -O3 -Wall")

file (STRINGS "version" BUILD_NUMBER)
project(timeswipe VERSION ${BUILD_NUMBER} DESCRIPTION "timeswipe driver")
if (${EMUL})
SET(CMAKE_C_COMPILER gcc )
SET(CMAKE_CXX_COMPILER g++ )
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
SET(CMAKE_C_COMPILER gcc )
SET(CMAKE_CXX_COMPILER g++ )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
SET(CMAKE_C_COMPILER aarch64-rpi3-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-rpi3-linux-gnu-g++ )
SET(CMAKE_AR aarch64-rpi3-linux-gnu-ar )
SET(CMAKE_C_FLAGS "-lpthread -fPIC")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -fPIC")
set(CMAKE_OSX_SYSROOT "")
set(CMAKE_RANLIB "echo")
elseif (${ARM32})
SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++ )
else ()
SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
endif ()
SET(CMAKE_INSTALL_LIBDIR lib)

if(DEFINED CC)
    SET(CMAKE_C_COMPILER ${CC})
endif()
if(DEFINED CXX)
    SET(CMAKE_CXX_COMPILER ${CXX})
endif()

include(GNUInstallDirs)

SET(firmware_dir ../firmware)
SET(firmware_includes ${firmware_dir}/src/Interfaces ${firmware_dir}/src/Communication ${firmware_dir}/src/HATS_EEPROM ../3rdParty/HATS_EEPROM)

SET(SRC
    src/timeswipe.cpp
    src/timeswipe_eeprom.cpp
    src/timeswipe_event.cpp
    src/timeswipe_resampler.cpp
    src/pidfile.cpp
    src/board_iface.cpp
    ../3rdParty/BCMsrc/bcm2835.c
    src/RaspberryPi/bcmspi.cpp
    ${firmware_dir}/src/HATS_EEPROM/HatsMemMan.cpp
)


add_library(timeswipeStatic STATIC ${SRC})
set_target_properties(timeswipeStatic PROPERTIES OUTPUT_NAME "timeswipe")
set_target_properties(timeswipeStatic PROPERTIES PUBLIC_HEADER include/timeswipe.hpp)
set_target_properties(timeswipeStatic PROPERTIES VERSION ${PROJECT_VERSION})
SET(timeswipe_include_dirs src src/RaspberryPi ${firmware_includes} src/Interfaces src/Communication ../3rdParty ../3rdParty/BCMsrc ../3rdParty/nlohmann/single_include include)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
execute_process(COMMAND brew --prefix boost OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE osx_boost)
SET(timeswipe_include_dirs ${timeswipe_include_dirs} ${osx_boost}/include)
endif ()
target_include_directories(timeswipeStatic PRIVATE ${timeswipe_include_dirs})
set_target_properties(timeswipeStatic PROPERTIES CXX_STANDARD 17)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
else ()
add_library(timeswipe SHARED ${SRC})
set_target_properties(timeswipe PROPERTIES PUBLIC_HEADER include/timeswipe.hpp)
set_target_properties(timeswipe PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(timeswipe PROPERTIES SOVERSION 1)
target_include_directories(timeswipe PRIVATE ${timeswipe_include_dirs})
set_target_properties(timeswipe PROPERTIES CXX_STANDARD 17)
endif ()

find_library(ATOMIC_LIB atomic NO_DEFAULT_PATH PATHS /usr/lib/gcc/arm-linux-gnueabihf/8)
if(ATOMIC_LIB)
    target_link_libraries(timeswipe ${ATOMIC_LIB})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
else ()
SET(CMAKE_C_FLAGS "-lpthread -O3 -fPIC -Wall")
SET(CMAKE_CXX_FLAGS "-fpermissive -lpthread -O3 -fPIC -std=gnu++17 -Wall -Wno-psabi")
SET(CMAKE_INSTALL_PREFIX "/usr")
install(TARGETS timeswipe timeswipeStatic
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(contrib/timeswipe.pc.in timeswipe.pc @ONLY)
configure_file(contrib/DEBIAN/control.in DEBIAN/control @ONLY)
configure_file(contrib/arch/PKGBUILD.in PKGBUILD @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/timeswipe.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
endif()
