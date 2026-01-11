# src/core/CMakeLists.txt

add_library(codex_core STATIC
    services/TextParser.cpp
    services/PromptBuilder.cpp
    services/MythicClassifier.cpp
    services/NarrationCleaner.cpp
    entities/GnosticEntities.cpp
    controllers/PipelineController.cpp
)

target_include_directories(codex_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(codex_core PUBLIC
    codex_utils
    codex_api
    Qt6::Core
    nlohmann_json::nlohmann_json
)
