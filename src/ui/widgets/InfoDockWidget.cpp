#include "InfoDockWidget.h"

#include <QVBoxLayout>
#include <QLabel>

namespace codex::ui {

InfoDockWidget::InfoDockWidget(QWidget* parent)
    : QDockWidget("Informations API", parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetClosable |
                QDockWidget::DockWidgetMovable |
                QDockWidget::DockWidgetFloatable);
    setupUi();
}

void InfoDockWidget::setupUi() {
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(5, 5, 5, 5);

    m_tabWidget = new QTabWidget(container);

    // Pricing tab
    m_pricingBrowser = new QTextBrowser(m_tabWidget);
    m_pricingBrowser->setOpenExternalLinks(true);
    m_pricingBrowser->setHtml(getVertexAIPricing());
    m_tabWidget->addTab(m_pricingBrowser, "Tarifs");

    // Quotas tab
    m_quotasBrowser = new QTextBrowser(m_tabWidget);
    m_quotasBrowser->setOpenExternalLinks(true);
    m_quotasBrowser->setHtml(getQuotasInfo());
    m_tabWidget->addTab(m_quotasBrowser, "Quotas");

    // Tips tab
    m_tipsBrowser = new QTextBrowser(m_tabWidget);
    m_tipsBrowser->setOpenExternalLinks(true);
    m_tipsBrowser->setHtml(getTipsInfo());
    m_tabWidget->addTab(m_tipsBrowser, "Conseils");

    layout->addWidget(m_tabWidget);
    setWidget(container);

    // Set minimum size
    setMinimumWidth(300);
    setMinimumHeight(200);
}

QString InfoDockWidget::getVertexAIPricing() const {
    return R"(
<html>
<head>
<style>
body { font-family: 'Segoe UI', sans-serif; font-size: 11px; color: #d4d4d4; background-color: #1e1e1e; padding: 10px; }
h2 { color: #4fc3f7; margin-top: 15px; margin-bottom: 8px; font-size: 14px; }
h3 { color: #81c784; margin-top: 12px; margin-bottom: 5px; font-size: 12px; }
table { border-collapse: collapse; width: 100%; margin: 8px 0; }
th, td { border: 1px solid #3d3d3d; padding: 6px 8px; text-align: left; }
th { background-color: #094771; color: white; }
tr:nth-child(even) { background-color: #262626; }
.price { color: #ffb74d; font-weight: bold; }
.free { color: #81c784; }
a { color: #4fc3f7; }
.note { color: #888; font-style: italic; font-size: 10px; }
</style>
</head>
<body>

<h2>Tarifs Google Vertex AI (2026)</h2>

<h3>Gemini 3 Pro</h3>
<table>
<tr><th>Type</th><th>Prix (≤200K)</th><th>Prix (>200K)</th></tr>
<tr><td>Input tokens</td><td class="price">$2.00 / 1M</td><td class="price">$4.00 / 1M</td></tr>
<tr><td>Output tokens</td><td class="price">$12.00 / 1M</td><td class="price">$18.00 / 1M</td></tr>
</table>
<p class="note">Context: 1M tokens input, 64K output max</p>

<h3>Gemini 3 Flash</h3>
<table>
<tr><th>Type</th><th>Prix</th></tr>
<tr><td>Input tokens</td><td class="price">$0.50 / 1M tokens</td></tr>
<tr><td>Output tokens</td><td class="price">$3.00 / 1M tokens</td></tr>
</table>
<p class="note">Tier gratuit disponible dans Gemini API</p>

<h3>Gemini 2.0 Flash</h3>
<table>
<tr><th>Type</th><th>Prix</th></tr>
<tr><td>Input tokens</td><td class="price">$0.15 / 1M tokens</td></tr>
<tr><td>Output tokens</td><td class="price">$0.60 / 1M tokens</td></tr>
</table>

<h3>Imagen 4.0 (Generation d'images)</h3>
<table>
<tr><th>Modele</th><th>Prix/image</th></tr>
<tr><td>imagen-4.0-generate-001</td><td class="price">~$0.039</td></tr>
<tr><td>imagen-4.0-fast-generate-001</td><td class="price">~$0.02</td></tr>
<tr><td>imagen-4.0-ultra-generate-001</td><td class="price">~$0.08</td></tr>
</table>
<p class="note">Base: $30/1M tokens, 1 image 1024px = 1,290 tokens</p>

<h3>Veo 2/3 (Generation video)</h3>
<table>
<tr><th>Service</th><th>Prix/seconde</th></tr>
<tr><td>Vertex AI (Veo 2)</td><td class="price">$0.50/sec</td></tr>
<tr><td>Gemini API (Veo 2)</td><td class="price">$0.35/sec</td></tr>
<tr><td>fal.ai (Veo 3.1 Fast)</td><td class="price">~$0.10/sec</td></tr>
</table>
<p class="note">Video 5 sec Vertex AI = ~$2.50</p>

<h3>AI Studio (Gratuit)</h3>
<table>
<tr><th>Service</th><th>Limite</th></tr>
<tr><td>Gemini 3 Flash</td><td class="free">Tier gratuit disponible</td></tr>
<tr><td>Gemini 2.0 Flash</td><td class="free">15 RPM, 1M TPM</td></tr>
<tr><td>Imagen 3</td><td class="free">Limite quotidienne</td></tr>
</table>

<p><a href="https://cloud.google.com/vertex-ai/generative-ai/pricing">Documentation officielle</a></p>

</body>
</html>
)";
}

QString InfoDockWidget::getQuotasInfo() const {
    return R"(
<html>
<head>
<style>
body { font-family: 'Segoe UI', sans-serif; font-size: 11px; color: #d4d4d4; background-color: #1e1e1e; padding: 10px; }
h2 { color: #4fc3f7; margin-top: 15px; margin-bottom: 8px; font-size: 14px; }
h3 { color: #81c784; margin-top: 12px; margin-bottom: 5px; font-size: 12px; }
table { border-collapse: collapse; width: 100%; margin: 8px 0; }
th, td { border: 1px solid #3d3d3d; padding: 6px 8px; text-align: left; }
th { background-color: #094771; color: white; }
tr:nth-child(even) { background-color: #262626; }
.limit { color: #ffb74d; }
a { color: #4fc3f7; }
.note { color: #888; font-style: italic; font-size: 10px; }
ul { margin: 5px 0; padding-left: 20px; }
li { margin: 3px 0; }
</style>
</head>
<body>

<h2>Quotas et Limites</h2>

<h3>Standard Pay-as-You-Go (PayGo)</h3>
<ul>
<li>Pas d'engagement, paiement a l'usage</li>
<li>Capacite ajustee selon depenses 30 jours</li>
<li>Limite systeme: <span class="limit">30,000 RPM</span> par modele/region</li>
</ul>

<h3>Tiers de debit (TPM - Tokens/minute)</h3>
<table>
<tr><th>Depenses 30j</th><th>Debit de base</th></tr>
<tr><td>$0 - $50</td><td class="limit">~100K TPM</td></tr>
<tr><td>$50 - $500</td><td class="limit">~500K TPM</td></tr>
<tr><td>$500+</td><td class="limit">1M+ TPM</td></tr>
</table>

<h3>AI Studio (Gratuit)</h3>
<table>
<tr><th>Modele</th><th>RPM</th><th>TPM</th></tr>
<tr><td>Gemini 3 Flash</td><td class="limit">Free tier</td><td class="limit">Disponible</td></tr>
<tr><td>Gemini 2.0 Flash</td><td class="limit">15</td><td class="limit">1,000,000</td></tr>
<tr><td>Gemini 3 Pro</td><td class="limit">Preview</td><td class="limit">Pas de tier gratuit</td></tr>
</table>

<h3>Express Mode (Sans facturation)</h3>
<ul>
<li>Vertex AI Studio et Agent Builder</li>
<li>Max 10 agent engines</li>
<li>Limite: 90 jours d'utilisation</li>
</ul>

<h3>Credits gratuits</h3>
<ul>
<li>Nouveaux utilisateurs: <span class="limit">$300</span> de credits</li>
<li>Validite: 90 jours</li>
</ul>

<h3>Erreur 429 (Rate Limit)</h3>
<p class="note">Ce n'est pas un quota fixe mais une contention temporaire.
Utilisez un backoff exponentiel pour reessayer.</p>

<p><a href="https://cloud.google.com/vertex-ai/generative-ai/docs/quotas">Documentation quotas</a></p>

</body>
</html>
)";
}

QString InfoDockWidget::getTipsInfo() const {
    return R"(
<html>
<head>
<style>
body { font-family: 'Segoe UI', sans-serif; font-size: 11px; color: #d4d4d4; background-color: #1e1e1e; padding: 10px; }
h2 { color: #4fc3f7; margin-top: 15px; margin-bottom: 8px; font-size: 14px; }
h3 { color: #81c784; margin-top: 12px; margin-bottom: 5px; font-size: 12px; }
.tip { background-color: #2d2d2d; border-left: 3px solid #4fc3f7; padding: 8px 12px; margin: 8px 0; }
.warning { background-color: #2d2d2d; border-left: 3px solid #ffb74d; padding: 8px 12px; margin: 8px 0; }
.success { background-color: #2d2d2d; border-left: 3px solid #81c784; padding: 8px 12px; margin: 8px 0; }
a { color: #4fc3f7; }
ul { margin: 5px 0; padding-left: 20px; }
li { margin: 3px 0; }
</style>
</head>
<body>

<h2>Conseils d'utilisation</h2>

<h3>Optimiser les couts</h3>

<div class="tip">
<b>AI Studio vs Vertex AI</b><br>
Utilisez AI Studio (gratuit) pour les tests et prototypes.
Passez a Vertex AI pour la production avec quotas plus eleves.
</div>

<div class="tip">
<b>Gemini API pour Veo</b><br>
Veo 2 via Gemini API: $0.35/sec (30% moins cher que Vertex AI a $0.50/sec)
</div>

<div class="success">
<b>Images: Imagen Fast</b><br>
Utilisez imagen-4.0-fast pour les previsualisations (~$0.02/image)
et Ultra pour les versions finales (~$0.08/image).
</div>

<h3>Eviter les erreurs</h3>

<div class="warning">
<b>Erreurs 429</b><br>
Implementez un retry avec backoff exponentiel.
Attendez 1s, puis 2s, puis 4s entre les tentatives.
</div>

<div class="tip">
<b>Tokens longs</b><br>
Les passages > 200K tokens sont factures au tarif "long context".
Divisez les textes longs en segments.
</div>

<h3>Bonnes pratiques</h3>

<ul>
<li>Testez les prompts sur AI Studio avant production</li>
<li>Utilisez des prompts courts et precis</li>
<li>Cachez les resultats pour eviter les requetes repetees</li>
<li>Surveillez vos depenses dans la console GCP</li>
</ul>

<h3>Liens utiles</h3>
<ul>
<li><a href="https://console.cloud.google.com/billing">Console de facturation GCP</a></li>
<li><a href="https://aistudio.google.com">Google AI Studio</a></li>
<li><a href="https://cloud.google.com/vertex-ai/docs/quotas">Demande d'augmentation quota</a></li>
</ul>

</body>
</html>
)";
}

} // namespace codex::ui
