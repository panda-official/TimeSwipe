#obtaining JSON settings from a package:

IF(NOT JSON_INCLUDE_DIR)

find_package(nlohmann_json 3.2.0 REQUIRED)

#strip the path:
get_target_property(JSON_INCLUDE_DIR nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Found a JSON path: ${JSON_INCLUDE_DIR}")
include_directories(${JSON_INCLUDE_DIR}) #one global inclusion


ENDIF()
