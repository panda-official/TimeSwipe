# -*- cmake -*-

# This macro must be used before project() command.
macro(panda_setup_build)
  if (PANDA_BUILD_ARM)
    set(sys_proc "armv7l")
    if (PANDA_BUILD_ARM_BARE_METAL)
      set(default_sys_name "Generic")
      set(default_c "arm-none-eabi-gcc")
      set(default_cpp "arm-none-eabi-g++")

      # We CMAKE_EXE_LINKER_FLAGS instead of CMAKE_EXE_LINKER_FLAGS_INIT since this
      # file is not actually pointed by CMAKE_TOOLCHAIN_FILE.
      set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs") # retarget to avoid linkage issues upon compiler testing
    else()
      set(default_sys_name "Linux")

      if(PANDA_BUILD_ARM64)
          set(sys_proc "aarch64")
          set(default_c "aarch64-linux-gnu-gcc")
          set(default_cpp "aarch64-linux-gnu-g++")
      else()
          set(default_c "arm-linux-gnueabihf-gcc")
          set(default_cpp "arm-linux-gnueabihf-g++")
      endif()

    endif()

    if (WIN32)
      foreach(tool default_c default_cpp)
        set(${tool} "${${tool}}.exe")
      endforeach()
    endif()

    if (NOT DEFINED CMAKE_SYSTEM_NAME)
      set(CMAKE_SYSTEM_NAME ${default_sys_name}) # CMake will set CMAKE_CROSSCOMPILING to True after this
    endif()
    if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
      set(CMAKE_SYSTEM_PROCESSOR ${default_sys_proc})
    endif()

    if (NOT DEFINED CMAKE_C_COMPILER)
      set(CMAKE_C_COMPILER ${default_c})
    endif()
    if (NOT DEFINED CMAKE_CXX_COMPILER)
      set(CMAKE_CXX_COMPILER ${default_cpp})
    endif()

    # CMAKE_PREFIX_PATH may point to the root directory
    # where the cross-compiled stuff like Boost is installed.
    if (NOT DEFINED CMAKE_PREFIX_PATH)
      list(INSERT CMAKE_PREFIX_PATH 0 "/opt/arm")
    endif()
  endif()
  add_compile_options(-Wno-psabi)
endmacro()
