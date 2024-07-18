cmake_minimum_required(VERSION 3.26)

project(
    GCBugFix
    VERSION 1.0
    LANGUAGES CXX
)

set(PROJECT_RESOURCES_PATH "${CMAKE_CURRENT_LIST_DIR}/res")
set(PROJECT_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/src")

#
# Set up the actual library
#
add_subdirectory("${PROJECT_SOURCE_PATH}")

#
# And finally produce build artifacts
#
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION_MAJOR}_${CMAKE_PROJECT_VERSION_MINOR}")

install(
    FILES
        "${PROJECT_RESOURCES_PATH}/GCBugFix.ini"
    DESTINATION
        "/"
)

install(
    TARGETS
        f4se_output_dll
    RUNTIME
        DESTINATION "Data/F4SE/Plugins/"
)

set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
