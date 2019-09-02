

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)

SET(COMPILER_PREFIX  "arm-none-eabi")

IF (WIN32)
    SET(EXEC_EXT ".exe")
ELSE()
    SET(EXEC_EXT "")
ENDIF()

SET(GCC_COMPILER_EXEC "${COMPILER_PREFIX}-gcc${EXEC_EXT}" )
SET(GPP_COMPILER_EXEC "${COMPILER_PREFIX}-g++${EXEC_EXT}" )

#trying to find a toolchain by default:
IF(NOT TC_BIN_PATH)
    LIST(APPEND FIND_LIST ${GCC_COMPILER_EXEC})
    find_path (TC_BIN_PATH NAMES ${FIND_LIST})
    message(STATUS "Found a toolchain path: ${TC_BIN_PATH}")
ENDIF()


IF(${CMAKE_VERSION} VERSION_LESS 3.6.0)
INCLUDE(CMakeForceCompiler)
    CMAKE_FORCE_C_COMPILER("${TC_BIN_PATH}/${GCC_COMPILER_EXEC}" GNU)
    CMAKE_FORCE_CXX_COMPILER("${TC_BIN_PATH}/${GPP_COMPILER_EXEC}" GNU)
ELSE()
    SET(CMAKE_C_COMPILER "${TC_BIN_PATH}/${GCC_COMPILER_EXEC}")
    SET(CMAKE_CXX_COMPILER "${TC_BIN_PATH}/${GPP_COMPILER_EXEC}")
    SET(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)               #this is very important for gnuarm!!!
ENDIF()
SET(CMAKE_ASM_COMPILER "${TC_BIN_PATH}/${GCC_COMPILER_EXEC}")



#fetch firmware dir:
string(REPLACE "src" "" PATH_FIRMWARE ${CMAKE_SOURCE_DIR})

#compiler flags:
set(COMPILER_FLAGS "-O0  -ffunction-sections -fdata-sections -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS} -std=c++17"  CACHE INTERNAL "")
set(CMAKE_ASM_FLAGS "${COMPILER_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "")

#linker flags:
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections  -mthumb -mcpu=cortex-m4 --specs=nosys.specs -T${PATH_FIRMWARE}/3rdParty/prj_templ/Custom/gcc.ld" CACHE INTERNAL "")

