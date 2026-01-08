#pragma once

#include <QDockWidget>
#include <QTextBrowser>
#include <QTabWidget>

namespace codex::ui {

class InfoDockWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit InfoDockWidget(QWidget* parent = nullptr);

private:
    void setupUi();
    QString getVertexAIPricing() const;
    QString getAIStudioPricing() const;
    QString getQuotasInfo() const;
    QString getTipsInfo() const;

    QTabWidget* m_tabWidget;
    QTextBrowser* m_pricingBrowser;
    QTextBrowser* m_quotasBrowser;
    QTextBrowser* m_tipsBrowser;
};

} // namespace codex::ui
