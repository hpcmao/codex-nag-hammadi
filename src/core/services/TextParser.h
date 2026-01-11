#pragma once

#include <QString>
#include <QVector>
#include <QStringList>
#include <QMap>

namespace codex::core {

struct TreatiseInfo {
    QString code;           // "I-1", "II-3", "8502-1", etc.
    QString pages;          // "A-B", "1-16", etc.
    QString title;          // "Prière de l'apôtre Paul"
    int startPage = 0;      // Page de début dans le fichier
    int endPage = 0;        // Page de fin
};

struct ParsedTreatise {
    QString code;
    QString title;
    QString fullText;
    QVector<QString> pages;
    QString category;       // Catégorie mythique
    int startPage = 0;      // Page de début (pour numérotation des versets)
};

struct ParsedPassage {
    QString text;
    int startPos;
    int endPos;
    QStringList detectedEntities;
};

class TextParser {
public:
    TextParser();

    // Charge le fichier Codex complet
    bool loadCodexFile(const QString& filePath);

    // Retourne true si un fichier est chargé
    bool isLoaded() const { return m_loaded; }

    // Retourne le contenu brut
    QString rawContent() const { return m_rawContent; }

    // Parse la table des matières et retourne la liste des traités
    QVector<TreatiseInfo> parseTableOfContents();

    // Extrait le contenu d'un traité spécifique
    ParsedTreatise extractTreatise(const QString& code);

    // Extrait tous les traités
    QVector<ParsedTreatise> parseAllTreatises();

    // Extrait un passage sélectionné
    ParsedPassage extractPassage(const QString& fullText, int start, int end);

    // Détecte les entités gnostiques dans le texte
    QStringList detectGnosticEntities(const QString& text);

    // Retourne le contenu d'une page spécifique
    QString getPageContent(int pageNumber);

    // Retourne le nombre total de pages
    int pageCount() const { return m_pages.size(); }

private:
    void loadEntityKeywords();
    void parsePages();
    void detectPageOffset();
    void parseByTitleHeaders();  // Détection alternative par titres (NH X, Y)
    QString normalizeCode(const QString& code) const;

    QString m_rawContent;
    QVector<QString> m_pages;           // Contenu par page
    QVector<TreatiseInfo> m_treatises;  // Table des matières parsée
    QMap<QString, QStringList> m_entityKeywords;
    int m_pageOffset = 0;               // Décalage entre pages TOC et pages fichier
    bool m_loaded = false;
};

} // namespace codex::core
