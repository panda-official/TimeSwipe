#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)


project(BOARD_lib)



file(GLOB BOARD_SRC "*.cpp")
add_library(BOARD_lib STATIC ${BOARD_SRC})

target_compile_definitions(BOARD_lib PRIVATE -D__SAME54P20A__)

target_include_directories(BOARD_lib PUBLIC .)
target_include_directories(BOARD_lib PUBLIC ../Math)
target_include_directories(BOARD_lib PUBLIC ../LEDs)
target_include_directories(BOARD_lib PUBLIC ../Interfaces)
target_include_directories(BOARD_lib PUBLIC ../ADCDAC)
target_include_directories(BOARD_lib PUBLIC ../JSONstuff)
target_include_directories(BOARD_lib PUBLIC ../Communication)
target_include_directories(BOARD_lib PUBLIC ../SAMe54)

target_include_directories(BOARD_lib PRIVATE ../../../3rdParty/prj_templ/include)
target_include_directories(BOARD_lib PRIVATE ../../../3rdParty/prj_templ/CMSIS/Include)


