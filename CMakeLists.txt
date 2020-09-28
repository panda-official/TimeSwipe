# -*- cmake -*-

cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

if (NOT UNIX)
  message("Only some of Unix-like platforms are supported")
endif()

# ------------------------------------------------------------------------------
# Modules
# ------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(panda)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------

set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL
  "Verbose output (useful to figure out what is going on at build time)?")

set(BUILD_SHARED_LIBS OFF CACHE BOOL
  "Build shared libraries?")

set(PANDA_BUILD_ARM OFF CACHE BOOL
  "Build for ARM?")

# ------------------------------------------------------------------------------
# Project
# ------------------------------------------------------------------------------

file(STRINGS driver/version VERSION_NUMBER
  LENGTH_MINIMUM 5 LENGTH_MAXIMUM 8 LIMIT_COUNT 1) # ab.cd.ef

panda_setup_build()

project(panda_timeswipe
  DESCRIPTION "Timeswipe driver"
  HOMEPAGE_URL https://github.com/panda-official/TimeSwipe
  VERSION ${VERSION_NUMBER}
)

# ------------------------------------------------------------------------------
# Languages
# ------------------------------------------------------------------------------

enable_language(CXX)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

# ------------------------------------------------------------------------------
# Output settings
# ------------------------------------------------------------------------------

# Note: Multi-configuration generators (VS, Xcode) appends a per-configuration
# subdirectory to CMAKE_RUNTIME_OUTPUT_DIRECTORY unless a generator expression
# is used.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

find_package(Boost)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found.
Hint: CMAKE_PREFIX_PATH can be used to point the root directory where it's installed.")
endif()

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

set(timeswipe_src
  driver/src/timeswipe.cpp
  driver/src/timeswipe_eeprom.cpp
  driver/src/timeswipe_event.cpp
  driver/src/timeswipe_resampler.cpp
  driver/src/pidfile.cpp
  driver/src/board_iface.cpp
  driver/src/RaspberryPi/bcmspi.cpp
  3rdParty/BCMsrc/bcm2835.c
  firmware/src/HATS_EEPROM/HatsMemMan.cpp
  )

add_library(timeswipe ${timeswipe_src})
target_link_libraries(timeswipe Boost::boost pthread)

if (NOT CMAKE_CROSSCOMPILING)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
      find_library(ATOMIC_LIB atomic NO_DEFAULT_PATH PATHS /usr/lib/gcc/arm-linux-gnueabihf/8)
      if (ATOMIC_LIB)
        message("Explicit Linking with atomic library")
        target_link_libraries(timeswipe ${ATOMIC_LIB})
      endif()
    endif()
  endif()
endif()

target_include_directories(timeswipe
  PRIVATE
  driver/src
  driver/src/RaspberryPi

  firmware/src/Interfaces
  firmware/src/Communication
  firmware/src/HATS_EEPROM

  driver/src/Interfaces
  driver/src/Communication

  3rdParty
  3rdParty/BCMsrc
  3rdParty/HATS_EEPROM

  PUBLIC
  driver/include
  )

configure_file(driver/contrib/timeswipe.pc.in driver/timeswipe.pc @ONLY)
configure_file(driver/contrib/DEBIAN/control.in driver/DEBIAN/control @ONLY)
configure_file(driver/contrib/arch/PKGBUILD.in driver/PKGBUILD @ONLY)

# ------------------------------------------------------------------------------
# Install
# ------------------------------------------------------------------------------

include(GNUInstallDirs)

install(TARGETS timeswipe
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY driver/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/driver/timeswipe.pc
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
