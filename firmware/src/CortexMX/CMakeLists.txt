#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)


project(CORTEXMX_lib)

add_library(CORTEXMX_lib STATIC nodeTimeCM4.cpp)

target_include_directories(CORTEXMX_lib PUBLIC ../Interfaces)
target_compile_definitions(CORTEXMX_lib PRIVATE -D__SAME54P20A__)
target_include_directories(CORTEXMX_lib PRIVATE ../../../3rdParty/prj_templ/include)
target_include_directories(CORTEXMX_lib PRIVATE ../../../3rdParty/prj_templ/CMSIS/Include)


