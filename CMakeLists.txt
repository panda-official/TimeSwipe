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

set(PANDA_TIMESWIPE_DOC_PDF Off CACHE BOOL
  "Build documentation PDFs?")

set(PANDA_TIMESWIPE_FIRMWARE Off CACHE BOOL
  "Build firmware? (Disables driver build.)")

set(PANDA_TIMESWIPE_FIRMWARE_SAM "E54P20A" CACHE STRING
  "Model of Atmel SAM controller to build firmware for.")

set(PANDA_TIMESWIPE_FIRMWARE_CALIBRATION Off CACHE BOOL
  "Build calibration firmware? (Disables driver build.)")

set(PANDA_TIMESWIPE_FIRMWARE_DMS On CACHE BOOL
  "Build firmware for the DMS board?")

set(PANDA_TIMESWIPE_FIRMWARE_RAM_IMAGE Off CACHE BOOL
  "Build firmware as RAM image?")

set(PANDA_TIMESWIPE_FIRMWARE_OPTIMIZE_DEBUG Off CACHE BOOL
  "Optimize debugging experience?")

set(PANDA_TIMESWIPE_TESTS On CACHE BOOL
  "Build tests (including examples)?")

if(PANDA_TIMESWIPE_FIRMWARE_CALIBRATION)
  set(PANDA_TIMESWIPE_FIRMWARE On)
endif()

if(PANDA_TIMESWIPE_FIRMWARE)
  set(PANDA_ARM On)
  set(PANDA_ARM_BARE_METAL On)

  # Some chips (like SAME53) might be too small to fit debug info without -Og.
  if(PANDA_TIMESWIPE_FIRMWARE_OPTIMIZE_DEBUG)
    foreach(cfg DEBUG RELWITHDEBINFO)
      foreach(lang ASM C CXX)
        set(CMAKE_${lang}_FLAGS_${cfg} "-g -Og")
      endforeach()
    endforeach()
  endif()

  if(PANDA_TIMESWIPE_FIRMWARE_SAM STREQUAL "E54P20A")
    set(sam_compile_definitions __SAME54P20A__)
    set(sam_include_directories src/3rdparty/same54/include)
    set(sam_startup_file "same54/gcc/startup_ARMCM4_E54.S")
    set(sam_sram_linker_file "src/3rdparty/same54/gcc/gcc_E54_RAM.ld")
    set(sam_flash_linker_file "src/3rdparty/same54/gcc/gcc_E54.ld")
    set(sam_max_flash_size 1048576)
    set(sam_max_ram_size 262144)
  elseif(PANDA_TIMESWIPE_FIRMWARE_SAM STREQUAL "E53N19A")
    set(sam_compile_definitions __SAME53N19A__)
    set(sam_include_directories src/3rdparty/same53/include)
    set(sam_startup_file "same53/gcc/startup_ARMCM4_E53.S")
    set(sam_sram_linker_file "src/3rdparty/same53/gcc/gcc_E53_RAM.ld")
    set(sam_flash_linker_file "src/3rdparty/same53/gcc/gcc_E53.ld")
    set(sam_max_flash_size 524288)
    set(sam_max_ram_size 196608)
  else()
    message(FATAL_ERROR "Unsupported model of SAM - ${PANDA_TIMESWIPE_FIRMWARE_SAM}")
  endif()
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

# Read software versions.
file(STRINGS .driver_version driver_version
  LENGTH_MINIMUM 5 LENGTH_MAXIMUM 8 LIMIT_COUNT 1) # ab.cd.ef
file(STRINGS .firmware_version firmware_version
  LENGTH_MINIMUM 5 LENGTH_MAXIMUM 8 LIMIT_COUNT 1) # ab.cd.ef

# Parse software versions.
foreach(software driver firmware)
  dmitigr_get_version(${${software}_version}
    ${software}_version_major ${software}_version_minor ${software}_version_patch)
endforeach()

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
  DESCRIPTION "Panda Timeswipe"
  HOMEPAGE_URL https://github.com/panda-official/TimeSwipe
  VERSION ${${software}_version_major} # Only major version sensible here.
)

if(CMAKE_CROSSCOMPILING)
  message("Cross-compiling for ${CMAKE_SYSTEM_PROCESSOR} ${CMAKE_SYSTEM_NAME}.")
endif()

# ------------------------------------------------------------------------------
# Languages
# ------------------------------------------------------------------------------

enable_language(C)
enable_language(CXX)
set(CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS Off)

# Parse C++ compiler version.
dmitigr_get_version(${CMAKE_CXX_COMPILER_VERSION}
  cxx_version_major cxx_version_minor cxx_version_patch)

# ------------------------------------------------------------------------------
# Output settings
# ------------------------------------------------------------------------------

# Note: Multi-configuration generators (VS, Xcode) appends a per-configuration
# subdirectory to CMAKE_RUNTIME_OUTPUT_DIRECTORY unless a generator expression
# is used.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
  set(panda_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
else()
  set(panda_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

# ------------------------------------------------------------------------------
# Documentation
# ------------------------------------------------------------------------------

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/firmware-api.md.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/doc/firmware-api.md"
  @ONLY
  NEWLINE_STYLE UNIX)

if(PANDA_TIMESWIPE_DOC_PDF)
  execute_process(COMMAND "pandoc" "-v" RESULT_VARIABLE pandoc_test_result
    ERROR_QUIET OUTPUT_QUIET)
  if(pandoc_test_result)
    message(FATAL_ERROR "Cannot generate PDF documentation: pandoc not found")
  else()
    message("Documentation PDFs will be written to: ${panda_output_dir}")
  endif()
endif()

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp"
  @ONLY
  NEWLINE_STYLE UNIX)

if(PANDA_TIMESWIPE_FIRMWARE)
  if(PANDA_TIMESWIPE_FIRMWARE_CALIBRATION)
    set(firmware_subtype "calibration")
  else()
    set(firmware_subtype "normal")
  endif()
  message(CHECK_START "Configuring the ${firmware_subtype} firmware build for SAM${PANDA_TIMESWIPE_FIRMWARE_SAM}")

  enable_language(ASM)

  if(PANDA_TIMESWIPE_FIRMWARE_RAM_IMAGE)
    set(linker_script ${sam_sram_linker_file})
  else()
    set(linker_script ${sam_flash_linker_file})
  endif()

  add_compile_options(
    -fno-exceptions
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
  set(adafruit_neopixel_src
    Adafruit_NeoPixel.cpp
    Adafruit_NeoPixel.h
    Adafruit_NeoPixel_stub.h
    esp8266.c
    )

  set(base_src
    DACPWM.cpp
    DACPWM.h
    DACPWMht.cpp
    DACPWMht.h
    FanControl.cpp
    FanControl.h
    FanControlSimple.cpp
    FanControlSimple.h
    I2Cmem8Pin.cpp
    I2Cmem8Pin.h
    I2CmemHAT.cpp
    I2CmemHAT.h
    OS_stub.cpp
    PINPWM.cpp
    PINPWM.h
    RawBinStorage.cpp
    RawBinStorage.h
    SPIcomm.cpp
    SPIcomm.h
    dac_max5715.cpp
    dac_max5715.hpp
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
    rgbacol.h
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
    SamDACcntr.cpp
    SamDACcntr.h
    SamDMAC.cpp
    SamDMAC.h
    SamI2Cmem.cpp
    SamI2Cmem.h
    SamNVMCTRL.cpp
    SamNVMCTRL.h
    SamQSPI.cpp
    SamQSPI.h
    SamSPIbase.cpp
    SamSPIbase.h
    SamService.cpp
    SamService.h
    SamTC.cpp
    SamTC.h
    SamTempSensor.cpp
    SamTempSensor.h
    button.cpp
    button.hpp
    cortex_handlers.cpp
    clock_generator.cpp
    clock_generator.hpp
    dsu.hpp
    dsu.cpp
    i2c_eeprom_master.cpp
    i2c_eeprom_master.hpp
    pin.cpp
    pin.hpp
    sercom.cpp
    sercom.hpp
    system_clock.cpp
    system_clock.hpp
    )

  set(firmware_src
    PWM.h
    Storage.h
    adcdac.hpp
    basics.hpp
    board.cpp
    board.hpp
    button.hpp
    channel.hpp
    dms_channel.cpp
    dms_channel.hpp
    error.hpp
    json.hpp
    os.h
    pga280.cpp
    pga280.hpp
    pin.hpp
    pin_button.hpp
    settings.hpp
    setting_handlers.hpp
    shiftreg.cpp
    shiftreg.hpp
    timer.h)
  list(TRANSFORM firmware_src PREPEND src/firmware/)
  foreach(dep base control led misc sam)
    list(TRANSFORM ${dep}_src PREPEND src/firmware/${dep}/)
    list(APPEND firmware_src ${${dep}_src})
  endforeach()

  # Third-party stuff
  ## Adafruit NeoPixel
  list(TRANSFORM adafruit_neopixel_src PREPEND src/3rdparty/adafruit/neopixel/)
  list(APPEND firmware_src ${adafruit_neopixel_src})

  add_library(firmware STATIC ${firmware_src})
  target_compile_definitions(firmware PUBLIC
    PANDA_TIMESWIPE_FIRMWARE

    # sam
    ${sam_compile_definitions}

    # Adafruit NeoPixel
    SAM_BRM __arm__  __SAMD51__
    )
  if(PANDA_TIMESWIPE_FIRMWARE_CALIBRATION)
    target_compile_definitions(firmware PUBLIC CALIBRATION_STATION)
  endif()
  if(PANDA_TIMESWIPE_FIRMWARE_DMS)
    target_compile_definitions(firmware PUBLIC DMS_BOARD)
  endif()
  target_include_directories(firmware PUBLIC src/firmware
    PRIVATE src/3rdparty/cmsis ${sam_include_directories}
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
  list(APPEND firmware_elf_3rdparty_src ${sam_startup_file})
  list(TRANSFORM firmware_elf_3rdparty_src PREPEND src/3rdparty/)
  list(APPEND firmware_elf_src ${firmware_elf_3rdparty_src})

  add_executable(${target_name} ${firmware_elf_src})
  target_link_libraries(${target_name} PRIVATE firmware)

  if(PANDA_TIMESWIPE_DOC_PDF)
    add_custom_target(firmware_doc_api ALL
      COMMAND "pandoc" "${CMAKE_CURRENT_SOURCE_DIR}/doc/firmware-api.md"
      "-o" "${panda_output_dir}/firmware-api.pdf"
      )
    add_dependencies(firmware_doc_api ${target_name})
  endif()

  add_custom_target(firmware_print_size ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tool/firmware-size.sh
    ${panda_output_dir}/${target_name} ${sam_max_flash_size} ${sam_max_ram_size}
    )
  # firmware_print_size must be the last target.
  if(PANDA_TIMESWIPE_DOC_PDF)
    add_dependencies(firmware_print_size firmware_doc_api)
  else()
    add_dependencies(firmware_print_size ${target_name})
  endif()

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
    src/driver.cpp
    src/3rdparty/bcm/bcm2835.c
    )

  add_library(panda_timeswipe STATIC ${timeswipe_src})

  list(APPEND timeswipe_link_libraries_public pthread)
  dmitigr_append_cppfs(timeswipe_link_libraries_public)
  target_link_libraries(panda_timeswipe
    PUBLIC ${timeswipe_link_libraries_public}
    PRIVATE Boost::boost)

  target_compile_definitions(panda_timeswipe PUBLIC
    )

  # When build natively on ARM with GCC 8+ link to atomic library explicitly if it's found.
  if(NOT CMAKE_CROSSCOMPILING)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
        set(atomic_lib_path "/usr/lib/gcc/arm-linux-gnueabihf/${cxx_version_major}")
        find_library(atomic_lib atomic NO_DEFAULT_PATH PATHS ${atomic_lib_path})
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
    src/basics.hpp
    src/board_settings.hpp
    src/driver.hpp
    src/driver_settings.hpp
    src/errc.hpp
    src/exceptions.hpp
    src/table.hpp
    src/types_fwd.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/panda/timeswipe)

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
  set(driver_tests bs board_settings driver_settings
    driftcomp driftcompmeas kaiser measure resampler rpispi
    table stop)
  set(firmware_tests button_event)

  # Set the link libraries per software.
  set(driver_tests_link_libraries panda_timeswipe)
  set(firmware_tests_link_libraries firmware)
  list(APPEND driver_tests_link_libraries)

  # Set the link libraries per test.
  set(resampler_link_libraries)

  # Set the extra sources per test.

  set(button_event_extra_sources src/3rdparty/${sam_startup_file})

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
      # DEBUG_POSTFIX "d"
      )
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
      )
  endif()

  message(CHECK_PASS "Tests configuration for ${software} completed.")
endif()
