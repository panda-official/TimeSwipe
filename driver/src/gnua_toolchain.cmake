#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)

SET(COMPILER_PREFIX  "aarch64-linux-gnu")

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
    IF(NOT CMAKE_C_COMPILER)
        CMAKE_FORCE_C_COMPILER("${TC_BIN_PATH}/${GCC_COMPILER_EXEC}" GNU)
    ELSE()
        CMAKE_FORCE_C_COMPILER(${CMAKE_C_COMPILER} GNU)
    ENDIF()
    IF(NOT CMAKE_CXX_COMPILER)
        CMAKE_FORCE_CXX_COMPILER("${TC_BIN_PATH}/${GPP_COMPILER_EXEC}" GNU)
    ELSE()
        CMAKE_FORCE_CXX_COMPILER(${CMAKE_CXX_COMPILER} GNU)
    ENDIF()
ELSE()
    IF(NOT CMAKE_C_COMPILER)
        SET(CMAKE_C_COMPILER "${TC_BIN_PATH}/${GCC_COMPILER_EXEC}")
    ENDIF()
    IF(NOT CMAKE_CXX_COMPILER)
        SET(CMAKE_CXX_COMPILER "${TC_BIN_PATH}/${GPP_COMPILER_EXEC}")
    ENDIF()
    SET(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)               #this is very important for gnuarm!!!
ENDIF()

SET(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER}) #05.09.2019


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS} -std=c++17"  CACHE INTERNAL "")

