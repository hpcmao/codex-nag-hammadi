#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "utils/MediaStorage.h"

namespace codex::ui {

class SessionPickerDialog : public QDialog {
    Q_OBJECT

public:
    enum ResultType {
        NewSession,
        ExistingSession
    };

    explicit SessionPickerDialog(QWidget* parent = nullptr);

    ResultType resultType() const { return m_resultType; }
    codex::utils::SessionInfo selectedSessionInfo() const { return m_selectedInfo; }
    QString selectedSessionPath() const { return m_selectedPath; }

private slots:
    void onNewSession();
    void onOpenSession();
    void onDeleteSession();
    void onSessionSelectionChanged();

private:
    void setupUi();
    void loadSessions();
    void updatePreview();

    QListWidget* m_sessionList;
    QLabel* m_previewLabel;
    QPushButton* m_newBtn;
    QPushButton* m_openBtn;
    QPushButton* m_deleteBtn;

    ResultType m_resultType = NewSession;
    codex::utils::SessionInfo m_selectedInfo;
    QString m_selectedPath;
};

} // namespace codex::ui

// For backward compatibility with code using unqualified name
using codex::ui::SessionPickerDialog;
