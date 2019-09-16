#This Source Code Form is subject to the terms of the GNU General Public License v3.0.
#If a copy of the GPL was not distributed with this
#file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
#Copyright (c) 2019 Panda Team


#obtaining JSON settings from a package:

IF(NOT JSON_INCLUDE_DIR)

find_package(nlohmann_json 3.2.0 REQUIRED)

#strip the path:
get_target_property(JSON_INCLUDE_DIR nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Found a JSON path: ${JSON_INCLUDE_DIR}")
include_directories(${JSON_INCLUDE_DIR}) #one global inclusion


ENDIF()
