# src/db/CMakeLists.txt

add_library(codex_db STATIC
    Database.cpp
    repositories/ProjectRepository.cpp
    repositories/PassageRepository.cpp
    repositories/ImageRepository.cpp
    repositories/AudioRepository.cpp
    repositories/FavoriteRepository.cpp
)

target_include_directories(codex_db PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(codex_db PUBLIC
    codex_utils
    Qt6::Core
    Qt6::Sql
)
