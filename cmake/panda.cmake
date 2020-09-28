# -*- cmake -*-

# This macro must be used before project() command.
macro(panda_setup_build)
  if (PANDA_BUILD_ARM)
    if (NOT DEFINED CMAKE_SYSTEM_NAME)
      set(CMAKE_SYSTEM_NAME "Linux") # CMake will set CMAKE_CROSSCOMPILING to True after this
    endif()
    if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
      set(CMAKE_SYSTEM_PROCESSOR "armv7l")
    endif()

    if (NOT DEFINED CMAKE_C_COMPILER)
      set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
    endif()
    if (NOT DEFINED CMAKE_CXX_COMPILER)
      set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")
    endif()

    # CMAKE_PREFIX_PATH may point to the root directory
    # where the cross-compiled stuff like Boost is installed.
    if (NOT DEFINED CMAKE_PREFIX_PATH)
      list(PREPEND CMAKE_PREFIX_PATH "/opt/arm")
    endif()
  endif()
  add_compile_options(-Wno-psabi)
endmacro()
