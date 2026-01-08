#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace codex::utils {

// Message box with selectable/copyable text
class MessageBox {
public:
    static void info(QWidget* parent, const QString& title, const QString& message) {
        showMessage(parent, title, message, QMessageBox::Information);
    }

    static void warning(QWidget* parent, const QString& title, const QString& message) {
        showMessage(parent, title, message, QMessageBox::Warning);
    }

    static void critical(QWidget* parent, const QString& title, const QString& message) {
        showMessage(parent, title, message, QMessageBox::Critical);
    }

    static QMessageBox::StandardButton question(QWidget* parent, const QString& title,
                                                 const QString& message,
                                                 QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No) {
        QMessageBox box(QMessageBox::Question, title, message, buttons, parent);
        box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        return static_cast<QMessageBox::StandardButton>(box.exec());
    }

private:
    static void showMessage(QWidget* parent, const QString& title, const QString& message, QMessageBox::Icon icon) {
        QMessageBox box(icon, title, message, QMessageBox::Ok, parent);
        box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        box.exec();
    }
};

} // namespace codex::utils
