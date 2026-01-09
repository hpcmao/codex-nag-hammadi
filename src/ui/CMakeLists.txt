# src/ui/CMakeLists.txt

add_library(codex_ui STATIC
    MainWindow.cpp
    widgets/TextViewerWidget.cpp
    widgets/ImageViewerWidget.cpp
    widgets/TreatiseListWidget.cpp
    widgets/PassagePreviewWidget.cpp
    widgets/AudioPlayerWidget.cpp
    widgets/SlideshowWidget.cpp
    widgets/InfoDockWidget.cpp
    dialogs/SettingsDialog.cpp
    dialogs/ProjectDialog.cpp
    dialogs/SlideshowDialog.cpp
)

target_include_directories(codex_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(codex_ui PUBLIC
    codex_core
    codex_api
    codex_db
    codex_utils
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
)
