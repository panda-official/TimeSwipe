#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team

cmake_minimum_required(VERSION 3.1)


project(COMM_lib)


file(GLOB COMM_SRC "*.cpp")
add_library(COMM_lib STATIC ${COMM_SRC} )

target_include_directories(COMM_lib PUBLIC ../Interfaces)



