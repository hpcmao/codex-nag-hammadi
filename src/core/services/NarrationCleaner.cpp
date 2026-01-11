#include "NarrationCleaner.h"

namespace codex::core {

NarrationCleaner::NarrationCleaner() {
    // Expressions régulières pré-compilées pour performance

    // ## Page 1, ## Page 2, etc.
    m_pageHeaderRegex = QRegularExpression(R"(^##\s*Page\s*\d+\s*$)",
        QRegularExpression::MultilineOption);

    // (NH I, 1), (NH II, 3), etc.
    m_codexRefRegex = QRegularExpression(R"(\s*\(NH\s+[IVX]+,?\s*\d+\))");

    // Traduction de ..., traduit par ...
    m_translatorRegex = QRegularExpression(
        R"(^(Traduction\s+de\s+[^\n]+|Traduit\s+par\s+[^\n]+)$)",
        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);

    // (Lacune de...), (lacune...), (Peut-être lacune...)
    m_lacunaRegex = QRegularExpression(
        R"(\(Lacune[^)]*\)|\(lacune[^)]*\)|\(Peut-être lacune[^)]*\))",
        QRegularExpression::CaseInsensitiveOption);

    // Lacunes avec points dans les crochets: [ . . . . ] ou [........]
    m_lacunaBracketsRegex = QRegularExpression(R"(\[[\s.]+\])");

    // Points espacés isolés: . . . . ou .....
    m_dotsSpacedRegex = QRegularExpression(R"(\.\s+\.\s+\.[\s.]*)");
    m_dotsManyRegex = QRegularExpression(R"(\.{4,})");

    // Numéros de lignes isolés sur leur propre ligne
    m_lineNumberRegex = QRegularExpression(R"(^\s*\d{1,3}\s*$)",
        QRegularExpression::MultilineOption);

    // Numéros de versets inline: "texte 10 Texte" -> "texte Texte"
    // Capture: espace + 1-2 chiffres + espace + lettre majuscule ou guillemet
    m_verseNumberInlineRegex = QRegularExpression(R"((\s)(\d{1,2})(\s)([A-Za-zÀ-ÿ«"]))");

    // [texte restauré] -> texte restauré (garde le contenu, enlève les crochets)
    m_bracketRegex = QRegularExpression(R"(\[([^\]]{1,60})\])");

    // (Décoration : ...), (Note*), (Note :...)
    m_decorationRegex = QRegularExpression(R"(\(Décoration\s*:[^)]*\))");
    m_noteRegex = QRegularExpression(R"(\(Note\s*\*?\s*:?[^)]*\)|\*(?=\s|$))");

    // Titres de traités en majuscules avec référence (NH X, Y)
    m_treatiseTitleRegex = QRegularExpression(
        R"(^[A-ZÉÈÊËÀÂÄÔÖÛÜÙÏÎÇ'\s\-<>]+\s*\(NH\s+[IVX]+,?\s*\d+\)\s*$)",
        QRegularExpression::MultilineOption);

    // Pipes | (changement de page dans le manuscrit)
    m_pipeRegex = QRegularExpression(R"(\s*\|\s*)");

    // Chevrons <texte> -> texte
    m_chevronRegex = QRegularExpression(R"(<([^<>]+)>)");

    // Espaces cassés dans les mots: "T able" -> "Table", "V érité" -> "Vérité"
    m_brokenWordRegex = QRegularExpression(R"(\b([A-ZÉÈÊËÀÂÄÔÖÛÜÙÏÎÇ])\s+([a-zéèêëàâäôöûüùïîç]))");
}

QString NarrationCleaner::clean(const QString& text) const {
    QString result = text;

    // 1. Supprimer les séparateurs markdown et headers de page
    if (m_removePageHeaders) {
        result = removePageHeaders(result);
    }
    result = removeSeparators(result);

    // 2. Supprimer les pipes (changement de page manuscrit)
    if (m_removePipes) {
        result = removePipes(result);
    }

    // 3. Supprimer les titres de traités avec référence NH
    if (m_removeTreatiseTitles) {
        result = removeTreatiseTitles(result);
    }

    // 4. Supprimer les références codex
    if (m_removeCodexRefs) {
        result = removeCodexReferences(result);
    }

    // 5. Supprimer les notes de traducteur
    if (m_removeTranslatorNotes) {
        result = removeTranslatorNotes(result);
    }

    // 6. Supprimer les indicateurs de lacune
    if (m_removeLacunaIndicators) {
        result = removeLacunaIndicators(result);
    }

    // 7. Supprimer décorations et notes
    result = removeDecorations(result);

    // 8. Supprimer les crochets (garde le contenu)
    if (m_removeBrackets) {
        result = removeBrackets(result);
    }

    // 9. Supprimer les chevrons (garde le contenu)
    result = removeChevrons(result);

    // 10. Supprimer les numéros de versets inline
    if (m_removeLineNumbers) {
        result = removeLineNumbers(result);
    }

    // 11. Corriger les espaces cassés dans les mots
    if (m_fixBrokenWords) {
        result = fixBrokenWords(result);
    }

    // 12. Supprimer les titres répétés en majuscules
    if (m_removeRepeatedTitles) {
        result = removeRepeatedTitles(result);
    }

    // 13. Normaliser les espaces en dernier
    result = normalizeWhitespace(result);

    return result;
}

QString NarrationCleaner::removePageHeaders(const QString& text) const {
    QString result = text;
    result.remove(m_pageHeaderRegex);
    return result;
}

QString NarrationCleaner::removeCodexReferences(const QString& text) const {
    QString result = text;
    result.remove(m_codexRefRegex);
    return result;
}

QString NarrationCleaner::removeTranslatorNotes(const QString& text) const {
    QString result = text;
    result.remove(m_translatorRegex);
    return result;
}

QString NarrationCleaner::removeLacunaIndicators(const QString& text) const {
    QString result = text;

    // Supprimer (Lacune de...) et variantes
    result.remove(m_lacunaRegex);

    // Supprimer les crochets avec points [ . . . . ]
    result.remove(m_lacunaBracketsRegex);

    // Remplacer les points espacés par "..."
    result.replace(m_dotsSpacedRegex, "... ");

    // Remplacer les points multiples par "..."
    result.replace(m_dotsManyRegex, "...");

    return result;
}

QString NarrationCleaner::removeLineNumbers(const QString& text) const {
    QString result = text;

    // Supprimer les numéros isolés sur leur propre ligne
    result.remove(m_lineNumberRegex);

    // Supprimer les numéros de versets inline (plusieurs passes)
    for (int i = 0; i < 3; ++i) {
        QString newResult = result;
        newResult.replace(m_verseNumberInlineRegex, "\\1\\4");
        if (newResult == result) break;
        result = newResult;
    }

    // Supprimer les numéros en fin de ligne
    QRegularExpression endLineNumbers(R"(\s+\d{1,2}\s*$)", QRegularExpression::MultilineOption);
    result.remove(endLineNumbers);

    return result;
}

QString NarrationCleaner::removeBrackets(const QString& text) const {
    QString result = text;

    // Plusieurs passes pour les crochets imbriqués
    for (int i = 0; i < 3; ++i) {
        QString newResult = result;
        newResult.replace(m_bracketRegex, "\\1");
        if (newResult == result) break;
        result = newResult;
    }

    return result;
}

QString NarrationCleaner::removeTreatiseTitles(const QString& text) const {
    QString result = text;
    result.remove(m_treatiseTitleRegex);
    return result;
}

QString NarrationCleaner::removeDecorations(const QString& text) const {
    QString result = text;
    result.remove(m_decorationRegex);
    result.remove(m_noteRegex);

    // Supprimer les lignes avec uniquement des symboles
    QRegularExpression symbolLines(R"(^[\s\+\*\#\-\=\†]+$)", QRegularExpression::MultilineOption);
    result.remove(symbolLines);

    return result;
}

QString NarrationCleaner::removeSeparators(const QString& text) const {
    QString result = text;

    // Supprimer les séparateurs markdown ---
    QRegularExpression separators(R"(^-{3,}$)", QRegularExpression::MultilineOption);
    result.remove(separators);

    return result;
}

QString NarrationCleaner::removePipes(const QString& text) const {
    QString result = text;
    result.replace(m_pipeRegex, " ");
    return result;
}

QString NarrationCleaner::removeChevrons(const QString& text) const {
    QString result = text;
    result.replace(m_chevronRegex, "\\1");
    return result;
}

QString NarrationCleaner::fixBrokenWords(const QString& text) const {
    QString result = text;
    // Corriger "T able" -> "Table", "V érité" -> "Vérité"
    result.replace(m_brokenWordRegex, "\\1\\2");
    return result;
}

QString NarrationCleaner::removeRepeatedTitles(const QString& text) const {
    // Supprimer les lignes en majuscules qui se répètent
    QStringList lines = text.split('\n');
    QStringList cleanedLines;
    QSet<QString> seenTitles;

    for (const QString& line : lines) {
        QString stripped = line.trimmed();

        // Ignorer les lignes vides
        if (stripped.isEmpty()) {
            cleanedLines.append(line);
            continue;
        }

        // Détecter les titres (tout en majuscules, > 5 caractères)
        bool isUpperCase = true;
        for (const QChar& c : stripped) {
            if (c.isLetter() && !c.isUpper()) {
                isUpperCase = false;
                break;
            }
        }

        if (isUpperCase && stripped.length() > 5) {
            QString normalized = stripped.left(30);
            if (seenTitles.contains(normalized)) {
                continue; // Skip répétition
            }
            seenTitles.insert(normalized);
        }

        cleanedLines.append(line);
    }

    return cleanedLines.join('\n');
}

QString NarrationCleaner::normalizeWhitespace(const QString& text) const {
    QString result = text;

    // Remplacer les espaces multiples par un seul
    QRegularExpression multipleSpaces(R"([ \t]+)");
    result.replace(multipleSpaces, " ");

    // Remplacer les lignes vides multiples par une seule
    QRegularExpression multipleNewlines(R"(\n\s*\n\s*\n+)");
    result.replace(multipleNewlines, "\n\n");

    // Supprimer les espaces en début/fin de ligne
    QRegularExpression lineEdgeSpaces(R"(^[ \t]+|[ \t]+$)", QRegularExpression::MultilineOption);
    result.remove(lineEdgeSpaces);

    // Trim global
    result = result.trimmed();

    return result;
}

} // namespace codex::core
