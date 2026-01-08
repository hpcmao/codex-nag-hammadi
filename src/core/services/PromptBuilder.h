#pragma once

#include <QString>
#include <QStringList>

namespace codex::core {

class PromptBuilder {
public:
    PromptBuilder();

    // Construit le prompt pour Claude API
    QString buildClaudePrompt(const QString& passageText,
                              const QStringList& entities,
                              const QString& category);

    // Assemble le prompt final pour Imagen
    QString buildImagenPrompt(const QString& sceneDescription,
                              const QString& emotion,
                              const QStringList& visualKeywords,
                              const QStringList& palette);

    // Template Villeneuve de base
    QString getVilleneuveTemplate() const;

private:
    QString m_baseTemplate;
    void loadTemplates();
};

} // namespace codex::core
