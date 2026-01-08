# CMakeLists.txt - Codex Nag Hammadi BD
# Application C++/Qt pour transformer les textes gnostiques en BD photoréaliste

cmake_minimum_required(VERSION 3.25)

project(CodexNagHammadi
    VERSION 1.0.0
    DESCRIPTION "BD photoréaliste des textes gnostiques de Nag Hammadi"
    LANGUAGES CXX
)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt automoc/autorcc/autouic
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Sql
    Multimedia
)

# Find nlohmann/json (optional - can be header-only)
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found via find_package, will use FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Sub-modules
add_subdirectory(src/utils)
add_subdirectory(src/core)
add_subdirectory(src/api)
add_subdirectory(src/db)
add_subdirectory(src/ui)

# Resources
set(RESOURCES
    resources/codex-nag-hammadi.qrc
)

# Main executable
add_executable(codex-nag-hammadi
    src/main.cpp
    ${RESOURCES}
)

target_link_libraries(codex-nag-hammadi PRIVATE
    codex_utils
    codex_core
    codex_api
    codex_db
    codex_ui
    Qt6::Core
    Qt6::Widgets
)

# Set executable properties
set_target_properties(codex-nag-hammadi PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "Codex Nag Hammadi"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# Installation
include(GNUInstallDirs)
install(TARGETS codex-nag-hammadi
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# CPack for packaging (optional)
set(CPACK_PACKAGE_NAME "CodexNagHammadi")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Hpcmao")
include(CPack)

# Summary
message(STATUS "")
message(STATUS "=== Codex Nag Hammadi BD ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
