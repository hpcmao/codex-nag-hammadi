#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>

namespace codex::core {

/**
 * @brief Nettoie le texte pour la narration vocale
 *
 * Supprime les éléments non narratifs comme :
 * - Numéros de page et références de codex
 * - Notes et indications de traduction
 * - Crochets autour des mots restaurés
 * - Numéros de versets/lignes inline
 * - Séparateurs, pipes et décorations
 * - Lacunes avec points espacés [ . . . ]
 * - Titres répétés en majuscules
 * - Espaces cassés dans les mots (T able -> Table)
 */
class NarrationCleaner {
public:
    NarrationCleaner();

    /**
     * @brief Nettoie le texte pour la narration
     * @param text Texte brut avec annotations
     * @return Texte nettoyé prêt pour la synthèse vocale
     */
    QString clean(const QString& text) const;

    /**
     * @brief Active/désactive des règles de nettoyage spécifiques
     */
    void setRemovePageHeaders(bool remove) { m_removePageHeaders = remove; }
    void setRemoveCodexReferences(bool remove) { m_removeCodexRefs = remove; }
    void setRemoveTranslatorNotes(bool remove) { m_removeTranslatorNotes = remove; }
    void setRemoveLacunaIndicators(bool remove) { m_removeLacunaIndicators = remove; }
    void setRemoveLineNumbers(bool remove) { m_removeLineNumbers = remove; }
    void setRemoveBrackets(bool remove) { m_removeBrackets = remove; }
    void setRemoveTreatiseTitles(bool remove) { m_removeTreatiseTitles = remove; }
    void setRemovePipes(bool remove) { m_removePipes = remove; }
    void setFixBrokenWords(bool fix) { m_fixBrokenWords = fix; }
    void setRemoveRepeatedTitles(bool remove) { m_removeRepeatedTitles = remove; }

private:
    QString removePageHeaders(const QString& text) const;
    QString removeCodexReferences(const QString& text) const;
    QString removeTranslatorNotes(const QString& text) const;
    QString removeLacunaIndicators(const QString& text) const;
    QString removeLineNumbers(const QString& text) const;
    QString removeBrackets(const QString& text) const;
    QString removeTreatiseTitles(const QString& text) const;
    QString removeDecorations(const QString& text) const;
    QString removeSeparators(const QString& text) const;
    QString removePipes(const QString& text) const;
    QString removeChevrons(const QString& text) const;
    QString fixBrokenWords(const QString& text) const;
    QString removeRepeatedTitles(const QString& text) const;
    QString normalizeWhitespace(const QString& text) const;

    bool m_removePageHeaders = true;
    bool m_removeCodexRefs = true;
    bool m_removeTranslatorNotes = true;
    bool m_removeLacunaIndicators = true;
    bool m_removeLineNumbers = true;
    bool m_removeBrackets = true;
    bool m_removeTreatiseTitles = true;
    bool m_removePipes = true;
    bool m_fixBrokenWords = true;
    bool m_removeRepeatedTitles = true;

    // Expressions régulières pré-compilées
    QRegularExpression m_pageHeaderRegex;
    QRegularExpression m_codexRefRegex;
    QRegularExpression m_translatorRegex;
    QRegularExpression m_lacunaRegex;
    QRegularExpression m_lacunaBracketsRegex;
    QRegularExpression m_dotsSpacedRegex;
    QRegularExpression m_dotsManyRegex;
    QRegularExpression m_lineNumberRegex;
    QRegularExpression m_verseNumberInlineRegex;
    QRegularExpression m_bracketRegex;
    QRegularExpression m_decorationRegex;
    QRegularExpression m_noteRegex;
    QRegularExpression m_treatiseTitleRegex;
    QRegularExpression m_pipeRegex;
    QRegularExpression m_chevronRegex;
    QRegularExpression m_brokenWordRegex;
};

} // namespace codex::core
