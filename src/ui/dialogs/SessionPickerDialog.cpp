#include "SessionPickerDialog.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileInfo>

namespace codex::ui {

SessionPickerDialog::SessionPickerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Sélectionner une session"));
    setMinimumSize(500, 400);
    setupUi();
    loadSessions();
}

void SessionPickerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Session list
    auto* listGroup = new QGroupBox(tr("Sessions existantes"));
    auto* listLayout = new QVBoxLayout(listGroup);

    m_sessionList = new QListWidget;
    m_sessionList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_sessionList, &QListWidget::itemSelectionChanged,
            this, &SessionPickerDialog::onSessionSelectionChanged);
    connect(m_sessionList, &QListWidget::itemDoubleClicked,
            this, &SessionPickerDialog::onOpenSession);
    listLayout->addWidget(m_sessionList);

    mainLayout->addWidget(listGroup);

    // Preview
    auto* previewGroup = new QGroupBox(tr("Aperçu"));
    auto* previewLayout = new QVBoxLayout(previewGroup);

    m_previewLabel = new QLabel(tr("Sélectionnez une session pour voir les détails"));
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setMinimumHeight(60);
    previewLayout->addWidget(m_previewLabel);

    mainLayout->addWidget(previewGroup);

    // Buttons
    auto* buttonLayout = new QHBoxLayout;

    m_deleteBtn = new QPushButton(tr("Supprimer"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &SessionPickerDialog::onDeleteSession);
    buttonLayout->addWidget(m_deleteBtn);

    buttonLayout->addStretch();

    m_newBtn = new QPushButton(tr("Nouvelle session"));
    m_newBtn->setDefault(true);
    connect(m_newBtn, &QPushButton::clicked, this, &SessionPickerDialog::onNewSession);
    buttonLayout->addWidget(m_newBtn);

    m_openBtn = new QPushButton(tr("Ouvrir"));
    m_openBtn->setEnabled(false);
    connect(m_openBtn, &QPushButton::clicked, this, &SessionPickerDialog::onOpenSession);
    buttonLayout->addWidget(m_openBtn);

    auto* cancelBtn = new QPushButton(tr("Annuler"));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    mainLayout->addLayout(buttonLayout);
}

void SessionPickerDialog::loadSessions() {
    m_sessionList->clear();

    auto& storage = codex::utils::MediaStorage::instance();
    QStringList sessions = storage.listSessions();

    for (const QString& sessionName : sessions) {
        QString sessionPath = storage.sessionsFolder() + "/" + sessionName;
        auto info = storage.loadSessionInfo(sessionPath);

        QString displayText = sessionName;
        if (!info.treatiseCode.isEmpty()) {
            displayText = QString("%1 (%2 images, %3 audio)")
                .arg(info.treatiseCode)
                .arg(info.imageCount)
                .arg(info.audioCount);
        } else {
            displayText = QString("%1 (%2 images)")
                .arg(sessionName)
                .arg(info.imageCount);
        }

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, sessionPath);
        m_sessionList->addItem(item);
    }

    if (sessions.isEmpty()) {
        m_previewLabel->setText(tr("Aucune session existante.\nCliquez sur 'Nouvelle session' pour commencer."));
    }
}

void SessionPickerDialog::onSessionSelectionChanged() {
    bool hasSelection = !m_sessionList->selectedItems().isEmpty();
    m_openBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);

    if (hasSelection) {
        updatePreview();
    }
}

void SessionPickerDialog::updatePreview() {
    auto* item = m_sessionList->currentItem();
    if (!item) return;

    QString sessionPath = item->data(Qt::UserRole).toString();
    auto& storage = codex::utils::MediaStorage::instance();
    auto info = storage.loadSessionInfo(sessionPath);

    QString preview = QString(
        "<b>Traité:</b> %1<br>"
        "<b>Date:</b> %2<br>"
        "<b>Images:</b> %3<br>"
        "<b>Audio:</b> %4"
    ).arg(info.treatiseCode.isEmpty() ? tr("(non spécifié)") : info.treatiseCode)
     .arg(info.timestamp)
     .arg(info.imageCount)
     .arg(info.audioCount);

    m_previewLabel->setText(preview);
}

void SessionPickerDialog::onNewSession() {
    m_resultType = NewSession;
    accept();
}

void SessionPickerDialog::onOpenSession() {
    auto* item = m_sessionList->currentItem();
    if (!item) return;

    m_selectedPath = item->data(Qt::UserRole).toString();
    auto& storage = codex::utils::MediaStorage::instance();
    m_selectedInfo = storage.loadSessionInfo(m_selectedPath);
    m_resultType = ExistingSession;

    LOG_INFO(QString("Opening session: %1").arg(m_selectedPath));
    accept();
}

void SessionPickerDialog::onDeleteSession() {
    auto* item = m_sessionList->currentItem();
    if (!item) return;

    QString sessionPath = item->data(Qt::UserRole).toString();
    QFileInfo fi(sessionPath);

    int ret = QMessageBox::question(this,
        tr("Confirmer la suppression"),
        tr("Voulez-vous vraiment supprimer la session '%1' ?\n\n"
           "Cette action est irréversible.").arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        auto& storage = codex::utils::MediaStorage::instance();
        if (storage.deleteSession(sessionPath)) {
            LOG_INFO(QString("Deleted session: %1").arg(sessionPath));
            loadSessions();
        } else {
            QMessageBox::warning(this,
                tr("Erreur"),
                tr("Impossible de supprimer la session."));
        }
    }
}

} // namespace codex::ui
