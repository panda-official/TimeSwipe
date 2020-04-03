#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)


project(PROCS_lib)


file(GLOB PROCS_SRC "*.cpp")
add_library(PROCS_lib STATIC ${PROCS_SRC})

target_include_directories(PROCS_lib PUBLIC ../Interfaces)
target_include_directories(PROCS_lib PUBLIC ../ADCDAC)
target_include_directories(PROCS_lib PUBLIC ../JSONstuff)
target_include_directories(PROCS_lib PUBLIC ../LEDs)
target_include_directories(PROCS_lib PUBLIC ../BusinessLogic)
target_include_directories(PROCS_lib PUBLIC ../Communication)
#target_include_directories(PROCS_lib PUBLIC ../BusinessLogic)
target_include_directories(PROCS_lib PUBLIC ../Board)
target_include_directories(PROCS_lib PUBLIC .)


