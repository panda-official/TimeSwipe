#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)


project(BLOGIC_lib)

add_library(BLOGIC_lib STATIC nodeControl.cpp DataVis.cpp View.cpp NewMenu.cpp SemVer.cpp)

target_include_directories(BLOGIC_lib PUBLIC ../Math)
target_include_directories(BLOGIC_lib PUBLIC ../Interfaces)
target_include_directories(BLOGIC_lib PUBLIC ../ADCDAC)
target_include_directories(BLOGIC_lib PUBLIC ../JSONstuff)
target_include_directories(BLOGIC_lib PUBLIC ../LEDs)
target_include_directories(BLOGIC_lib PUBLIC ../Board)
target_include_directories(BLOGIC_lib PUBLIC ../Procs)
target_include_directories(BLOGIC_lib PUBLIC ../Communication)
target_include_directories(PROCS_lib PUBLIC .)



