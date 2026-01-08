# src/utils/CMakeLists.txt

add_library(codex_utils STATIC
    Config.cpp
    Logger.cpp
    SecureStorage.cpp
    ThemeManager.cpp
)

target_include_directories(codex_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(codex_utils PUBLIC
    Qt6::Core
    Qt6::Widgets
    nlohmann_json::nlohmann_json
)

# Platform-specific libraries for SecureStorage
if(WIN32)
    target_link_libraries(codex_utils PRIVATE
        crypt32  # DPAPI
    )
elseif(APPLE)
    find_library(SECURITY_FRAMEWORK Security REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(codex_utils PRIVATE
        ${SECURITY_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()
