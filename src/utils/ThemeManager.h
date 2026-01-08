#pragma once

#include <QObject>
#include <QString>
#include <QColor>

namespace codex::utils {

struct ThemeColors {
    QString background;      // Main background
    QString backgroundAlt;   // Secondary background (toolbars, menus)
    QString backgroundPanel; // Panel backgrounds
    QString text;            // Primary text color
    QString textSecondary;   // Secondary/hint text
    QString accent;          // Primary accent (buttons, selection)
    QString accentHover;     // Accent on hover
    QString accentText;      // Text on accent background
    QString selection;       // Text selection background
    QString border;          // Borders and separators
    QString statusBar;       // Status bar background
    QString scrollHandle;    // Scrollbar handle
    QString error;           // Error color
    QString success;         // Success color
};

struct FontSettings {
    QString uiFamily = "Segoe UI";
    int uiSize = 10;
    QString textFamily = "Consolas";
    int textSize = 11;
};

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();

    // Theme management
    QString currentTheme() const { return m_themeName; }
    void setTheme(const QString& themeName);  // "dark" or "light"

    // Accent color
    QString accentColor() const { return m_accentColor; }
    void setAccentColor(const QString& color);

    // Font settings
    FontSettings fontSettings() const { return m_fonts; }
    void setFontSettings(const FontSettings& settings);

    // Get current colors
    ThemeColors colors() const { return m_colors; }

    // Generate complete stylesheet
    QString generateStyleSheet() const;

    // Apply theme to application (call after changes)
    void apply();

    // Load/save to config
    void load();
    void save();

    // Predefined themes
    static ThemeColors darkTheme();
    static ThemeColors lightTheme();

signals:
    void themeChanged();

private:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void applyAccentColor();
    QString adjustColor(const QString& baseColor, int lightenAmount) const;

    QString m_themeName = "dark";
    QString m_accentColor = "#094771";
    ThemeColors m_colors;
    FontSettings m_fonts;
};

} // namespace codex::utils
