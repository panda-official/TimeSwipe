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
include(misc)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------

set(CMAKE_VERBOSE_MAKEFILE Off CACHE BOOL
  "Verbose output (useful to figure out what is going on at build time)?")

set(PANDA_BUILD_FIRMWARE Off CACHE BOOL
  "Build firmware? (Disables driver build.)")

set(PANDA_BUILD_FIRMWARE_DMS On CACHE BOOL
  "Build firmware for the DMS board?")

set(PANDA_BUILD_FIRMWARE_EMU Off CACHE BOOL
  "Build firmware emulator?")

set(PANDA_BUILD_FIRMWARE_RAM_IMAGE Off CACHE BOOL
  "Build firmware as RAM image?")

set(PANDA_BUILD_CALIBRATION_FIRMWARE Off CACHE BOOL
  "Build calibration firmware?")

set(BUILD_SHARED_LIBS Off CACHE BOOL
  "Build shared libraries?")

set(PANDA_BUILD_ARM On CACHE BOOL
  "Build for ARM?")

set(PANDA_BUILD_ARM64 Off CACHE BOOL
  "Build for ARM64bit?")

set(PANDA_BUILD_TESTS On CACHE BOOL
  "Build tests (including examples)?")

if(PANDA_BUILD_FIRMWARE)
  set(PANDA_BUILD_ARM On)
  set(PANDA_BUILD_ARM_BARE_METAL On)

  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release")
  endif()
endif()

set(CMAKE_TOOLCHAIN_FILE "cmake/arm.cmake" CACHE PATH
  "Path to toolchain file.")

# ------------------------------------------------------------------------------
# Project
# ------------------------------------------------------------------------------

file(STRINGS driver/version VERSION_NUMBER
  LENGTH_MINIMUM 5 LENGTH_MAXIMUM 8 LIMIT_COUNT 1) # ab.cd.ef

# The `software` variable is a helper for substitution/interpolation.
if(PANDA_BUILD_FIRMWARE)
  set(software "firmware")
else()
  set(software "driver")
endif()

# Simple protection against changing the build target.
if((PANDA_BUILD_FIRMWARE AND EXISTS ${CMAKE_CURRENT_BINARY_DIR}/panda_build_driver)
    OR
   (NOT PANDA_BUILD_FIRMWARE AND EXISTS ${CMAKE_CURRENT_BINARY_DIR}/panda_build_firmware))
  message(FATAL_ERROR "Switching from driver build to firmware build and vice versa is forbidden!
Hint: please use different build directories for driver and firmware.")
else()
  file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/panda_build_${software})
endif()

project(panda_timeswipe
  DESCRIPTION "Timeswipe driver"
  HOMEPAGE_URL https://github.com/panda-official/TimeSwipe
  VERSION ${VERSION_NUMBER}
)

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
# Common dependencies
# ------------------------------------------------------------------------------

# No common dependencies

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

if(PANDA_BUILD_FIRMWARE_EMU)
  add_compile_definitions(PANDA_BUILD_FIRMWARE_EMU)
endif()

if(PANDA_BUILD_FIRMWARE)
  message(CHECK_START "Configuring the firmware build.")

  enable_language(ASM)

  if(PANDA_BUILD_FIRMWARE_RAM_IMAGE)
    set(linker_script "${CMAKE_SOURCE_DIR}/3rdParty/prj_templ/Custom/gcc_RAM.ld")
  else()
    set(linker_script "${CMAKE_SOURCE_DIR}/3rdParty/prj_templ/Custom/gcc.ld")
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
    -T${linker_script}
    )

  # Third-party bundled dependencies
  # Note: must follow add_{compile,link}_options()
  add_subdirectory(3rdParty)

  # ---------------------------
  # Internal dependency sources
  # ---------------------------

  set(ADCDAC_src
    ADC.h
    ADCDAC_doc.h
    ADchan.h
    DAC.cpp
    DAC.h
    )

  set(Board_src
    BaseMesChannel.h
    Board_doc.h
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

  set(BusinessLogic_src
    BusinessLogic_doc.h
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
  if(NOT PANDA_BUILD_FIRMWARE_EMU)
    list(APPEND BusinessLogic_src nodeControl.cpp)
  else()
    list(APPEND BusinessLogic_src nodeControl_emu.cpp)
  endif()

  set(Communication_src
    Communication_doc.h
    SyncCom.h
    cmd.cpp
    cmd.h
    frm_stream.cpp
    frm_stream.h
    std_port.cpp
    std_port.h
    )

  set(CortexMX_src
    CortexMX_doc.h
    nodeTimeCM4.cpp
    )

  set(HATS_EEPROM_src
    HatsMemMan.cpp
    HatsMemMan.h
    )

  set(Interfaces_src
    PWM.h
    Pin.h
    SPI.h
    Serial.h
    Storage.h
    button.h
    interfaces_doc.h
    os.h
    timer.h
    )

  set(JSONstuff_src
    JSONstuff_doc.h
    jsondisp.cpp
    jsondisp.h
    json_base.cpp
    json_base.h
    json_evsys.cpp
    json_evsys.h
    json_stream.cpp
    json_stream.h
    )

  set(LEDs_src
    Adafruit_NeoPixel_stub.h
    LEDs_doc.h
    ino_stub_SAME54.cpp
    nodeLED.cpp
    nodeLED.h
    )

  set(Math_src
    Math_doc.h
    mav.h
    ringbuffer.h
    )

  set(Procs_src
    ADpointSearch.cpp
    ADpointSearch.h
    Procs_doc.h
    zerocal_man.cpp
    zerocal_man.h
    )

  set(SAMe54_src
    NVMpage.h
    SAMe54_doc.h
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
    )

  foreach(dep ADCDAC Board BusinessLogic Communication CortexMX
      HATS_EEPROM Interfaces JSONstuff LEDs Math Procs SAMe54)
    list(TRANSFORM ${dep}_src PREPEND firmware/src/${dep}/)
  endforeach()

  # ---------------------------
  # Internal dependency targets
  # ---------------------------

  # Common to interface libraries
  foreach(dep Interfaces Math)
    add_library(${dep} INTERFACE)
    target_include_directories(${dep} INTERFACE firmware/src/${dep})
  endforeach()

  # Common to static libraries
  foreach(dep ADCDAC Board BusinessLogic Communication CortexMX
      HATS_EEPROM JSONstuff LEDs Procs SAMe54)
    add_library(${dep} OBJECT ${${dep}_src})
    target_include_directories(${dep} PUBLIC firmware/src/${dep})
  endforeach()

  # Communication-specific
  target_link_libraries(Communication PUBLIC Interfaces)

  # CortexMX-specific
  target_compile_definitions(CortexMX PRIVATE __SAME54P20A__)
  target_link_libraries(CortexMX PUBLIC Interfaces 3rdparty_CMSIS 3rdparty_sam)

  # SAMe54-specific
  target_compile_definitions(SAMe54 PRIVATE __SAME54P20A__)
  target_link_libraries(SAMe54 PUBLIC 3rdparty_CMSIS 3rdparty_sam ADCDAC Communication)

  # HATS_EEPROM-specific
  target_link_libraries(HATS_EEPROM PUBLIC Interfaces 3rdparty_HATS_EEPROM)

  # JSONstuff-specific
  target_link_libraries(JSONstuff PUBLIC 3rdparty_nlohmann_json Communication)

  # LEDs-specific
  target_link_libraries(LEDs PUBLIC Interfaces 3rdparty_CMSIS 3rdparty_sam
    PRIVATE 3rdparty_Adafruit_NeoPixel)

  # Board-specific
  target_compile_definitions(Board PRIVATE  __SAME54P20A__)
  target_link_libraries(Board PUBLIC Math 3rdparty_CMSIS 3rdparty_sam ADCDAC JSONstuff LEDs SAMe54)
  # FIXME: there is cyclical dependence: Board <--> BusinessLogic!!!
  # KLUDGE: for now just include directory of BusinessLogic
  target_include_directories(Board PRIVATE firmware/src/BusinessLogic)

  # BusinessLogic-specific
  target_link_libraries(BusinessLogic PUBLIC Board)
  # FIXME: there is cyclical dependence: BusinessLogic <--> Procs!!!
  # KLUDGE: for now just include directory of Procs
  target_include_directories(BusinessLogic PRIVATE firmware/src/Procs)

  # Procs-specific
  target_link_libraries(Procs PUBLIC BusinessLogic)

  # -------------------
  # firmware.elf target
  # -------------------

  if(PANDA_BUILD_CALIBRATION_FIRMWARE)
      SET(TARGET_NAME "calibration_firmware.elf")
      add_compile_definitions(CALIBRATION_STATION)
  else()
      SET(TARGET_NAME "firmware.elf")
  endif()


  set(firmware_src
    firmware/src/main.cpp
    3rdParty/prj_templ/Custom/startup_ARMCM4.S
    )

  add_executable(${TARGET_NAME} ${firmware_src})

  if(PANDA_BUILD_FIRMWARE_DMS)
    target_compile_definitions(${TARGET_NAME} PRIVATE DMS_BOARD)
  endif()

  target_link_libraries(${TARGET_NAME} ADCDAC Communication CortexMX
    HATS_EEPROM SAMe54 Board BusinessLogic JSONstuff LEDs Procs
    3rdparty_Adafruit_NeoPixel)

  message(CHECK_PASS "Ready to build the firmware.")

else()
  message(CHECK_START "Configuring the driver build.")

  # ------------
  # Dependencies
  # ------------

  find_package(Boost)
  if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found.
Hint: CMAKE_PREFIX_PATH can be used to point the root directory where it's installed.")
  endif()

  # Third-party bundled dependencies
  # Note: must follow add_{compile,link}_options()
  add_subdirectory(3rdParty)

  # ----------------
  # timeswipe target
  # ----------------

  # FIXME: firmware/src/HATS_EEPROM should be common library for both driver and firmware
  set(timeswipe_src
    driver/src/timeswipe.cpp
    driver/src/timeswipe_eeprom.cpp
    driver/src/timeswipe_event.cpp
    driver/src/timeswipe_resampler.cpp
    driver/src/pidfile.cpp
    driver/src/board_iface.cpp
    driver/src/RaspberryPi/bcmspi.cpp
    firmware/src/HATS_EEPROM/HatsMemMan.cpp # KLUDGE: see FIXME above
    )

  add_library(timeswipe ${timeswipe_src})
  target_link_libraries(timeswipe PUBLIC Boost::boost pthread 3rdparty_nlohmann_json 3rdparty_BCM)

  # When build natively on ARM with GCC 8+ link to atomic library explicitly if it's found.
  if(NOT CMAKE_CROSSCOMPILING)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
        find_library(atomic_lib atomic NO_DEFAULT_PATH PATHS /usr/lib/gcc/arm-linux-gnueabihf/8)
        if(atomic_lib)
          message("Explicit Linking with atomic library")
          target_link_libraries(timeswipe PUBLIC ${atomic_lib})
        endif()
      endif()
    endif()
  endif()

  target_include_directories(timeswipe
    PRIVATE
    driver/src/RaspberryPi

    firmware/src/Interfaces
    firmware/src/Communication
    firmware/src/HATS_EEPROM

    3rdParty
    3rdParty/BCMsrc
    3rdParty/HATS_EEPROM

    PUBLIC
    driver/src
    )

  # -----------------------
  # Package info generation
  # -----------------------

  configure_file(pkg/timeswipe.pc.in timeswipe.pc @ONLY)
  configure_file(pkg/DEBIAN/control.in DEBIAN/control @ONLY)
  configure_file(pkg/arch/PKGBUILD.in PKGBUILD @ONLY)

  # -------
  # Install
  # -------

  include(GNUInstallDirs)

  install(TARGETS timeswipe
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(FILES driver/src/timeswipe.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/timeswipe.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

  message(CHECK_PASS "Ready to build the driver.")
endif()

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(PANDA_BUILD_TESTS)
  enable_testing()
  message(CHECK_START "Configuring the tests for ${software}.")

  # Set the test lists.
  set(driver_tests datlog pwm rpispi)
  set(firmware_tests button_event)

  # Set the include directories per software.
  # FIXME: the best way to consume this stuff is target_link_libraries().
  set(driver_tests_include_directories
    driver/src/RaspberryPi
    firmware/src/Interfaces
    firmware/src/Communication)

  # Set the link libraries per software.
  set(driver_tests_link_libraries timeswipe)
  set(firmware_tests_link_libraries ADCDAC Board CortexMX JSONstuff LEDs SAMe54
    Interfaces Communication 3rdparty_Adafruit_NeoPixel)

  # Set the link libraries per test.
  set(datlog_link_libraries pthread)

  # Set the extra sources per test.
  set(button_event_extra_sources 3rdParty/prj_templ/Custom/startup_ARMCM4.S)

  # Configure the tests from the lists above.
  function(panda_configure_test test)
    set(full_name "${test}")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${software}/examples/${full_name}.cpp")
      set(is_unit_test TRUE)
      set(full_name "unit-${test}")
    else()
      set(is_unit_test FALSE)
    endif()
    set(exe "${full_name}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${software}/examples/${full_name}.cpp")
    add_executable(${exe} ${src} ${${test}_extra_sources})
    set_target_properties(${exe}
      PROPERTIES
      LINKER_LANGUAGE "CXX"
      POSITION_INDEPENDENT_CODE True
      DEBUG_POSTFIX "d")
    target_include_directories(${exe} PUBLIC
      ${${software}_tests_include_directories})
    target_link_libraries(${exe} PRIVATE
      ${${software}_tests_link_libraries} ${${test}_link_libraries})
    panda_target_compile_options(${exe})
    if(is_unit_test)
      add_test(NAME ${exe} COMMAND ${exe})
    endif()
  endfunction()
  foreach(test ${${software}_tests})
    panda_configure_test(${test})
  endforeach()

  # Custom commands
  if(NOT PANDA_BUILD_FIRMWARE)
    add_custom_target(panda_copy_test_resources ALL
      COMMAND cmake -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/driver/examples/datlog.json"
      "${panda_output_dir}"
      )
  endif()

  message(CHECK_PASS "Tests configuration for ${software} completed.")
endif()
