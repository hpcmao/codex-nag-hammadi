#include "TextParser.h"
#include "utils/Logger.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace codex::core {

TextParser::TextParser() {
    loadEntityKeywords();
}

void TextParser::loadEntityKeywords() {
    m_entityKeywords = {
        {"Plérôme", {"plérôme", "plénitude", "totalité divine", "pleroma"}},
        {"Sophia", {"sophia", "sagesse", "pistis", "pistis sophia"}},
        {"Éons", {"éon", "éons", "aiôn", "aiônes", "aeon"}},
        {"Archontes", {"archonte", "archontes", "dirigeants", "puissances"}},
        {"Démiurge", {"démiurge", "yaldabaoth", "saklas", "samael", "créateur"}},
        {"Christ", {"christ", "sauveur", "jésus", "seigneur"}},
        {"Père", {"père", "ineffable", "inconnaissable", "premier"}},
        {"Esprit", {"esprit", "souffle", "pneuma", "paraclet"}},
        {"Lumière", {"lumière", "lumineux", "clarté", "brillant", "resplendissant"}},
        {"Ténèbres", {"ténèbres", "obscurité", "noir", "ombre"}},
        {"Gnose", {"gnose", "connaissance", "savoir", "révélation"}},
        {"Âme", {"âme", "psychique", "psyché"}},
        {"Adam", {"adam", "premier homme"}},
        {"Ève", {"ève", "eve", "femme"}}
    };
}

bool TextParser::loadCodexFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open Codex file: %1").arg(filePath));
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    m_rawContent = in.readAll();
    file.close();

    parsePages();
    parseTableOfContents();

    m_loaded = true;
    LOG_INFO(QString("Loaded Codex file: %1 pages, %2 treatises")
             .arg(m_pages.size()).arg(m_treatises.size()));
    return true;
}

void TextParser::parsePages() {
    m_pages.clear();

    // Split by "## Page X" markers
    QRegularExpression pageRegex("## Page (\\d+)\\s*\\n");
    QRegularExpressionMatchIterator it = pageRegex.globalMatch(m_rawContent);

    QVector<int> pageStarts;
    QVector<int> pageNumbers;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        pageStarts.append(match.capturedEnd());
        pageNumbers.append(match.captured(1).toInt());
    }

    // Extract content between page markers
    for (int i = 0; i < pageStarts.size(); ++i) {
        int start = pageStarts[i];
        int end = (i + 1 < pageStarts.size()) ?
                  m_rawContent.lastIndexOf("---", pageStarts[i + 1]) :
                  m_rawContent.length();

        if (end < start) end = m_rawContent.length();

        QString pageContent = m_rawContent.mid(start, end - start).trimmed();

        // Remove trailing "---"
        if (pageContent.endsWith("---")) {
            pageContent.chop(3);
            pageContent = pageContent.trimmed();
        }

        // Ensure we have enough slots
        int pageNum = pageNumbers[i];
        while (m_pages.size() <= pageNum) {
            m_pages.append(QString());
        }
        m_pages[pageNum] = pageContent;
    }

    LOG_INFO(QString("Parsed %1 pages from Codex").arg(m_pages.size()));
}

QVector<TreatiseInfo> TextParser::parseTableOfContents() {
    if (!m_treatises.isEmpty()) {
        return m_treatises;
    }

    m_treatises.clear();

    // La table des matières est sur les pages 7-8
    // Format: "I–1 A-B   Prière de l'apôtre Paul . . . . . . . . . . . . 1"
    // ou "II–2 32-51 L'Évangile selon Thomas . . . . . . . . . . 101"

    QString tocContent;
    for (int i = 7; i <= 8 && i < m_pages.size(); ++i) {
        tocContent += m_pages[i] + "\n";
    }

    // Regex pour parser les entrées de la table des matières
    // Capture: code (I-1), pages manuscrit (A-B ou 1-16), titre, page fichier
    QRegularExpression tocRegex(
        R"(([IVX]+–\d+|8502–\d+|X)\s+(\d+\*?-\d+\*?|A-B)\s+(.+?)\s*\.{2,}\s*(\d+))"
    );

    QRegularExpressionMatchIterator it = tocRegex.globalMatch(tocContent);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        TreatiseInfo info;
        info.code = normalizeCode(match.captured(1));
        info.pages = match.captured(2);
        info.title = match.captured(3).trimmed();
        info.startPage = match.captured(4).toInt();

        // Clean title (remove L', Le, La, Les prefixes artifacts)
        info.title = info.title.replace(QRegularExpression("^L '"), "L'");
        info.title = info.title.replace(QRegularExpression("^L'"), "L'");

        m_treatises.append(info);
    }

    // Calculer les pages de fin (page de début du suivant - 1)
    for (int i = 0; i < m_treatises.size() - 1; ++i) {
        m_treatises[i].endPage = m_treatises[i + 1].startPage - 1;
    }
    if (!m_treatises.isEmpty()) {
        m_treatises.last().endPage = m_pages.size() - 1;
    }

    LOG_INFO(QString("Parsed %1 treatises from table of contents").arg(m_treatises.size()));
    return m_treatises;
}

QString TextParser::normalizeCode(const QString& code) const {
    QString normalized = code;
    // Remplacer "–" (tiret long) par "-" (tiret normal)
    normalized.replace("–", "-");
    return normalized;
}

ParsedTreatise TextParser::extractTreatise(const QString& code) {
    ParsedTreatise result;
    QString normalizedCode = normalizeCode(code);

    // Trouver le traité dans la table des matières
    TreatiseInfo* info = nullptr;
    for (auto& t : m_treatises) {
        if (t.code == normalizedCode) {
            info = &t;
            break;
        }
    }

    if (!info) {
        LOG_WARN(QString("Treatise not found: %1").arg(code));
        return result;
    }

    result.code = info->code;
    result.title = info->title;

    // Extraire le contenu des pages
    QStringList contentParts;
    for (int p = info->startPage; p <= info->endPage && p < m_pages.size(); ++p) {
        QString pageContent = m_pages[p];
        if (!pageContent.isEmpty()) {
            contentParts.append(pageContent);
            result.pages.append(pageContent);
        }
    }

    result.fullText = contentParts.join("\n\n");

    return result;
}

QVector<ParsedTreatise> TextParser::parseAllTreatises() {
    QVector<ParsedTreatise> results;

    for (const auto& info : m_treatises) {
        ParsedTreatise treatise = extractTreatise(info.code);
        if (!treatise.fullText.isEmpty()) {
            results.append(treatise);
        }
    }

    return results;
}

QString TextParser::getPageContent(int pageNumber) {
    if (pageNumber >= 0 && pageNumber < m_pages.size()) {
        return m_pages[pageNumber];
    }
    return QString();
}

ParsedPassage TextParser::extractPassage(const QString& fullText, int start, int end) {
    ParsedPassage passage;
    passage.startPos = start;
    passage.endPos = end;
    passage.text = fullText.mid(start, end - start);
    passage.detectedEntities = detectGnosticEntities(passage.text);
    return passage;
}

QStringList TextParser::detectGnosticEntities(const QString& text) {
    QStringList detected;
    QString lowerText = text.toLower();

    for (auto it = m_entityKeywords.constBegin(); it != m_entityKeywords.constEnd(); ++it) {
        for (const QString& keyword : it.value()) {
            if (lowerText.contains(keyword.toLower())) {
                if (!detected.contains(it.key())) {
                    detected.append(it.key());
                }
                break;
            }
        }
    }

    return detected;
}

} // namespace codex::core
