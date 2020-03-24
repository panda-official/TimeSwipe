#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

#SAME54 sub-dir lib:

cmake_minimum_required(VERSION 3.1)


project(SAME54_lib)



file(GLOB SAME54_SRC "*.cpp")
add_library(SAME54_lib STATIC ${SAME54_SRC})

target_compile_definitions(SAME54_lib PRIVATE -D__SAME54P20A__)


target_include_directories(SAME54_lib PUBLIC ../Interfaces)
target_include_directories(SAME54_lib PUBLIC ../ADCDAC)
target_include_directories(SAME54_lib PUBLIC ../Communication)

target_include_directories(SAME54_lib PRIVATE ../../../3rdParty/prj_templ/include)
target_include_directories(SAME54_lib PRIVATE ../../../3rdParty/prj_templ/CMSIS/Include)


