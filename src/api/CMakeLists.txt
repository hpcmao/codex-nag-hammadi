# src/api/CMakeLists.txt

add_library(codex_api STATIC
    ApiClient.cpp
    ClaudeClient.cpp
    GeminiClient.cpp
    ImagenClient.cpp
    VeoClient.cpp
    ElevenLabsClient.cpp
)

target_include_directories(codex_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(codex_api PUBLIC
    codex_utils
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    nlohmann_json::nlohmann_json
)
