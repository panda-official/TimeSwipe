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

# ------------------------------------------------------------------------------
# dmitigr_misc
# ------------------------------------------------------------------------------

if(NOT PANDA_BUILD_FIRMWARE)
  add_library(3rdparty_dmitigr_misc INTERFACE)
  target_include_directories(3rdparty_dmitigr_misc INTERFACE dmitigr)
endif()

# ------------------------------------------------------------------------------
# nlohmann_json
# ------------------------------------------------------------------------------

add_library(3rdparty_nlohmann_json INTERFACE)
target_compile_definitions(3rdparty_nlohmann_json INTERFACE NLOHMANN_JSON_CUSTOM_STDIO_FRM)
target_include_directories(3rdparty_nlohmann_json INTERFACE nlohmann)

# ------------------------------------------------------------------------------
# HATS_EEPROM
# ------------------------------------------------------------------------------

add_library(3rdparty_HATS_EEPROM INTERFACE)
target_include_directories(3rdparty_HATS_EEPROM INTERFACE HATS_EEPROM)

if(PANDA_BUILD_FIRMWARE)
  # ------------------------------------------------------------------------------
  # Adafruit_NeoPixel
  # ------------------------------------------------------------------------------

  set(Adafruit_NeoPixel_src
    Adafruit_NeoPixel/Adafruit_NeoPixel.cpp
    Adafruit_NeoPixel/Adafruit_NeoPixel.h
    ${CMAKE_SOURCE_DIR}/src/firmware/led/Adafruit_NeoPixel_stub.h # FIXME!
    Adafruit_NeoPixel/esp8266.c
    )

  add_library(3rdparty_Adafruit_NeoPixel OBJECT ${Adafruit_NeoPixel_src})
  target_compile_definitions(3rdparty_Adafruit_NeoPixel PUBLIC SAM_BRM __arm__  __SAMD51__  __SAME54P20A__)
  target_include_directories(3rdparty_Adafruit_NeoPixel PUBLIC Adafruit_NeoPixel
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common # FIXME!
    ${CMAKE_SOURCE_DIR}/src/firmware) # FIXME!

  # ------------------------------------------------------------------------------
  # CMSIS
  # ------------------------------------------------------------------------------

  add_library(3rdparty_CMSIS INTERFACE)
  target_include_directories(3rdparty_CMSIS INTERFACE prj_templ/CMSIS/Include)

  # ------------------------------------------------------------------------------
  # sam
  # ------------------------------------------------------------------------------

  add_library(3rdparty_sam INTERFACE)
  target_include_directories(3rdparty_sam INTERFACE prj_templ/include)
else()
  # ------------------------------------------------------------------------------
  # BCM
  # ------------------------------------------------------------------------------

  if (NOT PANDA_BUILD_FIRMWARE)
    set(BCM_src
      BCMsrc/bcm2835.c
      BCMsrc/bcm2835.h
      )

    add_library(3rdparty_BCM OBJECT ${BCM_src})
    target_include_directories(3rdparty_BCM PUBLIC BCMsrc)
  endif()
endif()
