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

if(PANDA_BUILD_ARM)
  if(PANDA_BUILD_ARM64)
    set(default_sys_proc "aarch64")
  else()
    set(default_sys_proc "armv7l")
  endif()

  if(PANDA_BUILD_ARM_BARE_METAL)
    set(default_sys_name "Generic")
    if(PANDA_BUILD_ARM64)
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
    if(PANDA_BUILD_ARM64)
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
