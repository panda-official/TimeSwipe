# -*- cmake -*-
# Copyright (C) 2021  PANDA GmbH

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

# ------------------------------------------------------------------------------
# Modules
# ------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(dmitigr)
include(misc)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------

set(PANDA_CLANG_USE_LIBCPP On CACHE BOOL
  "Use libcpp with Clang?")

set(PANDA_ARM On CACHE BOOL
  "Build for ARM?")

set(PANDA_ARM64 Off CACHE BOOL
  "Build for ARM64?")

set(PANDA_TIMESWIPE_FIRMWARE Off CACHE BOOL
  "Build firmware? (Disables driver build.)")

set(PANDA_TIMESWIPE_CALIBRATION_FIRMWARE Off CACHE BOOL
  "Build calibration firmware? (Disables driver build.)")

set(PANDA_TIMESWIPE_FIRMWARE_DMS On CACHE BOOL
  "Build firmware for the DMS board?")

set(PANDA_TIMESWIPE_FIRMWARE_EMU Off CACHE BOOL
  "Build firmware emulator?")

set(PANDA_TIMESWIPE_FIRMWARE_RAM_IMAGE Off CACHE BOOL
  "Build firmware as RAM image?")

set(PANDA_TIMESWIPE_TESTS On CACHE BOOL
  "Build tests (including examples)?")

if(PANDA_TIMESWIPE_CALIBRATION_FIRMWARE)
  set(PANDA_TIMESWIPE_FIRMWARE On)
endif()

if(PANDA_TIMESWIPE_FIRMWARE)
  set(PANDA_ARM On)
  set(PANDA_ARM_BARE_METAL On)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "The type of build")
endif()

set(CMAKE_VERBOSE_MAKEFILE On CACHE BOOL
  "Verbose output (useful to figure out what is going on at build time)?")

set(CMAKE_TOOLCHAIN_FILE "cmake/arm.cmake" CACHE PATH
  "Path to toolchain file.")

# ------------------------------------------------------------------------------
# Project
# ------------------------------------------------------------------------------

file(STRINGS .version version
  LENGTH_MINIMUM 5 LENGTH_MAXIMUM 8 LIMIT_COUNT 1) # ab.cd.ef

# The `software` variable is a helper for substitution/interpolation.
if(PANDA_TIMESWIPE_FIRMWARE)
  set(software "firmware")
else()
  set(software "driver")
endif()

# Simple protection against changing the build target.
if((PANDA_TIMESWIPE_FIRMWARE AND EXISTS ${CMAKE_CURRENT_BINARY_DIR}/panda_build_driver)
    OR
   (NOT PANDA_TIMESWIPE_FIRMWARE AND EXISTS ${CMAKE_CURRENT_BINARY_DIR}/panda_build_firmware))
  message(FATAL_ERROR "Switching from driver build to firmware build and vice versa is forbidden!
Hint: please use different build directories for driver and firmware.")
else()
  file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/panda_build_${software})
endif()

project(panda_timeswipe
  DESCRIPTION "Timeswipe driver"
  HOMEPAGE_URL https://github.com/panda-official/TimeSwipe
  VERSION ${version}
)

if(CMAKE_CROSSCOMPILING)
  message("Cross-compiling for ${CMAKE_SYSTEM_PROCESSOR} ${CMAKE_SYSTEM_NAME}.")
endif()

# ------------------------------------------------------------------------------
# Languages
# ------------------------------------------------------------------------------

enable_language(CXX)
set(CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS Off)

# ------------------------------------------------------------------------------
# Output settings
# ------------------------------------------------------------------------------

# Note: Multi-configuration generators (VS, Xcode) appends a per-configuration
# subdirectory to CMAKE_RUNTIME_OUTPUT_DIRECTORY unless a generator expression
# is used.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if (WIN32)
  set(panda_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
elseif (UNIX)
  set(panda_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

if(PANDA_TIMESWIPE_FIRMWARE_EMU)
  add_compile_definitions(PANDA_TIMESWIPE_FIRMWARE_EMU)
endif()

if(PANDA_TIMESWIPE_FIRMWARE)
  if(PANDA_TIMESWIPE_CALIBRATION_FIRMWARE)
    set(firmware_subtype "calibration")
  else()
    set(firmware_subtype "normal")
  endif()
  message(CHECK_START "Configuring the ${firmware_subtype} firmware build.")

  enable_language(ASM)

  if(PANDA_TIMESWIPE_FIRMWARE_RAM_IMAGE)
    set(linker_script "src/3rdparty/prj_templ/Custom/gcc_RAM.ld")
  else()
    set(linker_script "src/3rdparty/prj_templ/Custom/gcc.ld")
  endif()

  add_compile_options(
    -ffunction-sections
    -fdata-sections
    -mthumb
    -mcpu=cortex-m4
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    # -x assembler-with-cpp
    )

  add_link_options(
    LINKER:--gc-sections
    -mthumb
    -mcpu=cortex-m4
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    # --specs=nosys.specs
    # --specs=nano.specs
    -T${CMAKE_SOURCE_DIR}/${linker_script}
    )

  # --------------------------------
  # firmware target (static library)
  # --------------------------------

  # 3rdparty
  set(Adafruit_NeoPixel_src
    Adafruit_NeoPixel.cpp
    Adafruit_NeoPixel.h
    Adafruit_NeoPixel_stub.h
    esp8266.c
    )

  set(base_src
    BaseMesChannel.h
    DACPWM.cpp
    DACPWM.h
    DACPWMht.cpp
    DACPWMht.h
    DACmax5715.cpp
    DACmax5715.h
    DMSchannel.cpp
    DMSchannel.h
    FanControl.cpp
    FanControl.h
    FanControlSimple.cpp
    FanControlSimple.h
    I2Cmem8Pin.cpp
    I2Cmem8Pin.h
    I2CmemHAT.cpp
    I2CmemHAT.h
    OS_stub.cpp
    PGA280.cpp
    PGA280.h
    PINPWM.cpp
    PINPWM.h
    RawBinStorage.cpp
    RawBinStorage.h
    SAMbutton.cpp
    SAMbutton.h
    SPIcomm.cpp
    SPIcomm.h
    ShiftReg.cpp
    ShiftReg.h
    pin_button.h
    )

  set(control_src
    ADpointSearch.cpp
    ADpointSearch.h
    zerocal_man.cpp
    zerocal_man.h

    CalFWbtnHandler.h
    DataVis.cpp
    DataVis.h
    NewMenu.cpp
    NewMenu.h
    SemVer.cpp
    SemVer.h
    View.cpp
    View.h
    board_type.h
    nodeControl.h
    rgbacol.h
    )
  if(NOT PANDA_TIMESWIPE_FIRMWARE_EMU)
    list(APPEND control_src nodeControl.cpp)
  else()
    list(APPEND control_src nodeControl_emu.cpp)
  endif()

  set(json_src
    jsondisp.cpp
    jsondisp.h
    json_base.cpp
    json_base.h
    json_evsys.cpp
    json_evsys.h
    json_stream.cpp
    json_stream.h
    )

  set(led_src
    Adafruit_NeoPixel_stub.cpp
    nodeLED.cpp
    nodeLED.h
    )

  set(misc_src
    mav.h
    ringbuffer.h
    )

  set(sam_src
    NVMpage.h
    SamADCcntr.cpp
    SamADCcntr.h
    SamCLK.cpp
    SamCLK.h
    SamDACcntr.cpp
    SamDACcntr.h
    SamDMAC.cpp
    SamDMAC.h
    SamI2CeepromMaster.cpp
    SamI2CeepromMaster.h
    SamI2Cmem.cpp
    SamI2Cmem.h
    SamNVMCTRL.cpp
    SamNVMCTRL.h
    SamPORT.cpp
    SamPORT.h
    SamQSPI.cpp
    SamQSPI.h
    SamSPIbase.cpp
    SamSPIbase.h
    SamSercom.cpp
    SamSercom.h
    SamService.cpp
    SamService.h
    SamTC.cpp
    SamTC.h
    SamTempSensor.cpp
    SamTempSensor.h
    clock_set_SAME54.cpp
    cortex_handlers.cpp
    )

  # Our stuff
  set(firmware_src adcdac.hpp error.hpp)
  list(TRANSFORM firmware_src PREPEND src/firmware/)
  foreach(dep base control json led misc sam)
    list(TRANSFORM ${dep}_src PREPEND src/firmware/${dep}/)
    list(APPEND firmware_src ${${dep}_src})
  endforeach()

  # Third-party stuff
  foreach(dep Adafruit_NeoPixel)
    list(TRANSFORM ${dep}_src PREPEND src/3rdparty/${dep}/)
    list(APPEND firmware_src ${${dep}_src})
  endforeach()

  add_library(firmware STATIC ${firmware_src})
  target_compile_definitions(firmware PUBLIC
    # Nlohmann JSON
    NLOHMANN_JSON_CUSTOM_STDIO_FRM

    # prj_templ
    __SAME54P20A__

    # Adafruit_NeoPixel
    SAM_BRM __arm__  __SAMD51__  __SAME54P20A__
    )
  if(PANDA_TIMESWIPE_CALIBRATION_FIRMWARE)
    target_compile_definitions(firmware PUBLIC CALIBRATION_STATION)
  endif()
  if(PANDA_TIMESWIPE_FIRMWARE_DMS)
    target_compile_definitions(firmware PUBLIC DMS_BOARD)
  endif()
  target_include_directories(firmware PUBLIC src/firmware
    PRIVATE
    src/3rdparty/prj_templ/include
    src/3rdparty/prj_templ/CMSIS/Include
    )

  # -------------------
  # firmware.elf target
  # -------------------

  set(firmware_name "firmware")
  set(target_name "${firmware_subtype}_firmware.elf")

  # Primary firmware.elf sources
  set(firmware_elf_src
    ${firmware_name}.cpp
    )
  list(TRANSFORM firmware_elf_src PREPEND src/firmware/)

  # Third-party firmware.elf sources
  list(APPEND firmware_elf_3rdparty_src
    prj_templ/Custom/startup_ARMCM4.S
    )
  list(TRANSFORM firmware_elf_3rdparty_src PREPEND src/3rdparty/)
  list(APPEND firmware_elf_src ${firmware_elf_3rdparty_src})

  add_executable(${target_name} ${firmware_elf_src})
  target_link_libraries(${target_name} PRIVATE firmware)

  message(CHECK_PASS "Ready to build the ${firmware_subtype} firmware.")
else()
  message(CHECK_START "Configuring the driver build.")

  # ------------
  # Dependencies
  # ------------

  # Boost
  find_package(Boost)
  if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found.
Hint: CMAKE_PREFIX_PATH can be used to point the root directory where it's installed.")
  endif()

  # ----------------
  # timeswipe target
  # ----------------

  set(timeswipe_src
    src/driver/eeprom.cpp
    src/driver/event.cpp
    src/driver/gpio.cpp
    src/driver/pidfile.cpp
    src/driver/sensor_data.cpp
    src/driver/timeswipe.cpp

    # BCM
    src/3rdparty/BCMsrc/bcm2835.c
    )

  add_library(panda_timeswipe STATIC ${timeswipe_src})

  list(APPEND timeswipe_link_libraries_public pthread)
  dmitigr_append_cppfs(timeswipe_link_libraries_public)
  target_link_libraries(panda_timeswipe
    PUBLIC ${timeswipe_link_libraries_public}
    PRIVATE Boost::boost)

  target_compile_definitions(panda_timeswipe PUBLIC
    # Nlohmann JSON
    NLOHMANN_JSON_CUSTOM_STDIO_FRM
    )

  # When build natively on ARM with GCC 8+ link to atomic library explicitly if it's found.
  if(NOT CMAKE_CROSSCOMPILING)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
        find_library(atomic_lib atomic NO_DEFAULT_PATH PATHS /usr/lib/gcc/arm-linux-gnueabihf/8)
        if(atomic_lib)
          message("Explicit Linking with atomic library.")
          target_link_libraries(panda_timeswipe PUBLIC ${atomic_lib})
        endif()
      endif()
    endif()
  endif()

  # -----------------------
  # Package info generation
  # -----------------------

  configure_file(pkg/panda_timeswipe.pc.in panda_timeswipe.pc @ONLY)
  configure_file(pkg/DEBIAN/control.in DEBIAN/control @ONLY)
  configure_file(pkg/arch/PKGBUILD.in PKGBUILD @ONLY)

  # -------
  # Install
  # -------

  include(GNUInstallDirs)

  install(TARGETS panda_timeswipe
    EXPORT timeswipe_export
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(EXPORT timeswipe_export
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/panda_timeswipe/cmake
    FILE panda_timeswipe-config.cmake)

  install(FILES
    src/driver.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda/timeswipe)

  install(FILES
    src/driver/error.hpp
    src/driver/timeswipe.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda/timeswipe/driver)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/panda_timeswipe.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

  message(CHECK_PASS "Ready to build the driver.")
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(PANDA_TIMESWIPE_TESTS)
  enable_testing()
  message(CHECK_START "Configuring the tests for ${software}.")

  # Set the test lists.
  set(driver_tests datlog driftcomp driftcompmeas kaiser measure pwm resampler rpispi)
  set(firmware_tests button_event)

  # Set the link libraries per software.
  set(driver_tests_link_libraries panda_timeswipe)
  set(firmware_tests_link_libraries firmware)
  list(APPEND driver_tests_link_libraries)

  # Set the link libraries per test.
  set(datlog_link_libraries pthread)
  set(resampler_link_libraries)

  # Set the extra sources per test.
  set(button_event_extra_sources src/3rdparty/prj_templ/Custom/startup_ARMCM4.S)

  # Configure the tests from the lists above.
  function(panda_configure_test test)
    set(full_name "${test}")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/${software}/${full_name}.cpp")
      set(is_unit_test TRUE)
      set(full_name "unit-${test}")
    else()
      set(is_unit_test FALSE)
    endif()
    set(exe "${full_name}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/test/${software}/${full_name}.cpp")
    add_executable(${exe} ${src} ${${test}_extra_sources})
    set_target_properties(${exe}
      PROPERTIES
      LINKER_LANGUAGE "CXX"
      POSITION_INDEPENDENT_CODE True
      DEBUG_POSTFIX "d")
    target_link_libraries(${exe}
      PRIVATE ${${software}_tests_link_libraries} ${${test}_link_libraries})
    panda_target_compile_options(${exe})
    if(is_unit_test)
      add_test(NAME ${exe} COMMAND ${exe})
    endif()
  endfunction()
  foreach(test ${${software}_tests})
    panda_configure_test(${test})
  endforeach()

  # Custom commands
  if(NOT PANDA_TIMESWIPE_FIRMWARE)
    add_custom_target(panda_copy_test_resources ALL
      COMMAND cmake -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/test/driver/datlog.json"
      "${panda_output_dir}"
      COMMAND cmake -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/test/driver/driftcompmeas.json"
      "${panda_output_dir}"
      )
  endif()

  message(CHECK_PASS "Tests configuration for ${software} completed.")
endif()
