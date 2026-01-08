#include <QApplication>
#include "ui/MainWindow.h"
#include "utils/Logger.h"
#include "utils/Config.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application info
    QApplication::setApplicationName("Codex Nag Hammadi BD");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Hpcmao");

    // Initialize logger
    codex::utils::Logger::instance().info("Application starting...");

    // Load configuration
    codex::utils::Config::instance().load();

    // Create and show main window
    codex::ui::MainWindow mainWindow;
    mainWindow.show();

    codex::utils::Logger::instance().info("Application ready");

    return app.exec();
}
