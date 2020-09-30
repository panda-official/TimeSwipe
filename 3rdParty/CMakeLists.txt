# -*- cmake -*-

# ------------------------------------------------------------------------------
# Adafruit_NeoPixel
# ------------------------------------------------------------------------------

set(Adafruit_NeoPixel_src
  Adafruit_NeoPixel/Adafruit_NeoPixel.cpp
  Adafruit_NeoPixel/Adafruit_NeoPixel.h
  ${CMAKE_SOURCE_DIR}/firmware/src/LEDs/Adafruit_NeoPixel_stub.h
  Adafruit_NeoPixel/esp8266.c
  )

add_library(3rdparty_Adafruit_NeoPixel OBJECT ${Adafruit_NeoPixel_src})
target_compile_definitions(3rdparty_Adafruit_NeoPixel PUBLIC SAM_BRM)
target_include_directories(3rdparty_Adafruit_NeoPixel PUBLIC Adafruit_NeoPixel
  PRIVATE
  ${CMAKE_SOURCE_DIR}/firmware/src/LEDs
  ${CMAKE_SOURCE_DIR}/firmware/src/Interfaces)

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

# ------------------------------------------------------------------------------
# HATS_EEPROM
# ------------------------------------------------------------------------------

add_library(3rdparty_HATS_EEPROM INTERFACE)
target_include_directories(3rdparty_HATS_EEPROM INTERFACE HATS_EEPROM)

# ------------------------------------------------------------------------------
# Upfirdn
# ------------------------------------------------------------------------------

add_library(3rdparty_Upfirdn INTERFACE)
target_include_directories(3rdparty_Upfirdn INTERFACE Upfirdn)

# ------------------------------------------------------------------------------
# nlohmann_json
# ------------------------------------------------------------------------------

add_library(3rdparty_nlohmann_json INTERFACE)
target_compile_definitions(3rdparty_nlohmann_json INTERFACE NLOHMANN_JSON_CUSTOM_STDIO_FRM)
target_include_directories(3rdparty_nlohmann_json INTERFACE nlohmann)

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
