#include "PromptBuilder.h"

namespace codex::core {

PromptBuilder::PromptBuilder() {
    loadTemplates();
}

void PromptBuilder::loadTemplates() {
    m_baseTemplate = "Cinematic, photoreal, Denis Villeneuve style, "
                     "dramatic lighting, desaturated colors, "
                     "monumental scale, contemplative atmosphere, "
                     "epic wide shot, 16:9 aspect ratio";
}

QString PromptBuilder::getVilleneuveTemplate() const {
    return m_baseTemplate;
}

QString PromptBuilder::buildClaudePrompt(const QString& passageText,
                                         const QStringList& entities,
                                         const QString& category) {
    QString prompt = QString(
        "Tu es un expert en textes gnostiques et en direction artistique visuelle.\n\n"
        "Analyse ce passage du Codex de Nag Hammadi et génère une description de scène visuelle.\n\n"
        "PASSAGE:\n%1\n\n"
        "ENTITÉS DÉTECTÉES: %2\n"
        "CATÉGORIE MYTHIQUE: %3\n\n"
        "Génère une réponse JSON avec:\n"
        "- scene: description de la scène visuelle (3-4 phrases)\n"
        "- emotion: l'émotion dominante\n"
        "- composition: suggestion de composition visuelle\n"
        "- visual_elements: liste de 5-7 éléments visuels clés"
    ).arg(passageText, entities.join(", "), category);

    return prompt;
}

QString PromptBuilder::buildImagenPrompt(const QString& sceneDescription,
                                         const QString& emotion,
                                         const QStringList& visualKeywords,
                                         const QStringList& palette) {
    QStringList parts;

    // Scene description
    parts << sceneDescription;

    // Villeneuve style base
    parts << m_baseTemplate;

    // Emotion
    if (!emotion.isEmpty()) {
        parts << QString("%1 mood").arg(emotion);
    }

    // Visual keywords
    if (!visualKeywords.isEmpty()) {
        parts << visualKeywords.join(", ");
    }

    // Color palette
    if (!palette.isEmpty()) {
        parts << QString("color palette: %1").arg(palette.join(", "));
    }

    return parts.join(". ");
}

} // namespace codex::core
