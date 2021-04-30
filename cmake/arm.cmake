# -*- cmake -*-
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin

if(PANDA_ARM)
  if(PANDA_ARM64)
    set(default_sys_proc "aarch64")
  else()
    set(default_sys_proc "armv7l")
  endif()

  if(PANDA_ARM_BARE_METAL)
    set(default_sys_name "Generic")
    if(PANDA_ARM64)
      set(default_c "aarch64-none-eabi-gcc")
      set(default_cpp "aarch64-none-eabi-g++")
    else()
      set(default_c "arm-none-eabi-gcc")
      set(default_cpp "arm-none-eabi-g++")
    endif()

    # Retarget to avoid linkage issues upon compiler testing.
    set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS}")
  else()
    set(default_sys_name "Linux")
    if(PANDA_ARM64)
      set(default_c "aarch64-linux-gnu-gcc")
      set(default_cpp "aarch64-linux-gnu-g++")
    else()
      set(default_c "arm-linux-gnueabihf-gcc")
      set(default_cpp "arm-linux-gnueabihf-g++")
    endif()
  endif()

  if(WIN32)
    foreach(tool default_c default_cpp)
      set(${tool} "${${tool}}.exe")
    endforeach()
  endif()

  if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER ${default_c})
  endif()
  if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER ${default_cpp})
  endif()

  # CMAKE_PREFIX_PATH may point to the root directory
  # where the cross-compiled stuff like Boost is installed.
  if(NOT DEFINED CMAKE_PREFIX_PATH)
    list(INSERT CMAKE_PREFIX_PATH 0 "/opt/arm")
  endif()

  if(NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
    set(CMAKE_SYSTEM_PROCESSOR ${default_sys_proc})
  endif()

  if(NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR)
    if(NOT DEFINED CMAKE_SYSTEM_NAME)
      # CMake automatically set CMAKE_CROSSCOMPILING to True if CMAKE_SYSTEM_NAME
      # was set manually. Note, that the effect of implicit modification of
      # CMAKE_CROSSCOMPILING is visible only after the project() CMake command.
      set(CMAKE_SYSTEM_NAME ${default_sys_name})
    endif()
  endif()
endif()

add_compile_options(-Wno-psabi)
