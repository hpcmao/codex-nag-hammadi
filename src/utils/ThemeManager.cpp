#include "ThemeManager.h"
#include "Config.h"
#include "Logger.h"

#include <QApplication>
#include <QColor>

namespace codex::utils {

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() {
    m_colors = darkTheme();
    load();
}

ThemeColors ThemeManager::darkTheme() {
    ThemeColors colors;
    colors.background = "#1e1e1e";
    colors.backgroundAlt = "#2d2d2d";
    colors.backgroundPanel = "#252525";
    colors.text = "#d4d4d4";
    colors.textSecondary = "#888888";
    colors.accent = "#094771";
    colors.accentHover = "#0a5a8c";
    colors.accentText = "#ffffff";
    colors.selection = "#000000";       // Noir pour fond de sélection
    colors.selectionText = "#00ff00";   // Vert pour texte sélectionné
    colors.border = "#3d3d3d";
    colors.statusBar = "#007acc";
    colors.scrollHandle = "#6a6a6a";
    colors.error = "#f44336";
    colors.success = "#4caf50";
    // Alternating rows - subtle contrast for readability
    colors.rowEven = "#1e1e1e";   // Same as background
    colors.rowOdd = "#262626";    // Slightly lighter
    return colors;
}

ThemeColors ThemeManager::lightTheme() {
    ThemeColors colors;
    colors.background = "#ffffff";
    colors.backgroundAlt = "#f5f5f5";
    colors.backgroundPanel = "#fafafa";
    colors.text = "#333333";
    colors.textSecondary = "#666666";
    colors.accent = "#0066cc";
    colors.accentHover = "#0088ff";
    colors.accentText = "#ffffff";
    colors.selection = "#000000";       // Noir pour fond de sélection
    colors.selectionText = "#00cc00";   // Vert pour texte sélectionné
    colors.border = "#dddddd";
    colors.statusBar = "#0066cc";
    colors.scrollHandle = "#c0c0c0";
    colors.error = "#d32f2f";
    colors.success = "#388e3c";
    // Alternating rows - subtle contrast for readability
    colors.rowEven = "#ffffff";   // Same as background
    colors.rowOdd = "#f8f8f8";    // Slightly darker
    return colors;
}

void ThemeManager::setTheme(const QString& themeName) {
    m_themeName = themeName;
    if (themeName == "light") {
        m_colors = lightTheme();
    } else {
        m_colors = darkTheme();
    }
    applyAccentColor();
    applyDamierColors();
    LOG_INFO(QString("Theme changed to: %1").arg(themeName));
}

void ThemeManager::setAccentColor(const QString& color) {
    m_accentColor = color;
    applyAccentColor();
    LOG_INFO(QString("Accent color changed to: %1").arg(color));
}

void ThemeManager::setSelectionColors(const QString& bgColor, const QString& textColor) {
    m_colors.selection = bgColor;
    m_colors.selectionText = textColor;
    LOG_INFO(QString("Selection colors changed: bg=%1, text=%2").arg(bgColor, textColor));
}

void ThemeManager::applyAccentColor() {
    m_colors.accent = m_accentColor;
    m_colors.accentHover = adjustColor(m_accentColor, 20);
    m_colors.statusBar = m_accentColor;
}

void ThemeManager::applyDamierColors() {
    if (!m_damier.enabled) {
        // Same color for both rows (no alternating)
        m_colors.rowOdd = m_colors.rowEven;
        return;
    }

    // Calculate odd row color based on contrast setting (0-100)
    QColor baseColor(m_colors.rowEven);
    int adjustment = m_damier.contrast / 4;  // Max ~25 units of lightness change

    if (m_themeName == "dark") {
        // Lighten for dark theme
        m_colors.rowOdd = adjustColor(m_colors.rowEven, adjustment);
    } else {
        // Darken for light theme
        m_colors.rowOdd = adjustColor(m_colors.rowEven, -adjustment);
    }
}

ThemeColors ThemeManager::colors() const {
    return m_colors;
}

void ThemeManager::setDamierSettings(const DamierSettings& settings) {
    m_damier = settings;
    applyDamierColors();
    LOG_INFO(QString("Damier settings changed: enabled=%1, contrast=%2")
             .arg(m_damier.enabled ? "true" : "false").arg(m_damier.contrast));
}

QString ThemeManager::adjustColor(const QString& baseColor, int lightenAmount) const {
    QColor color(baseColor);
    int h, s, l;
    color.getHsl(&h, &s, &l);
    l = qMin(255, l + lightenAmount);
    color.setHsl(h, s, l);
    return color.name();
}

void ThemeManager::setFontSettings(const FontSettings& settings) {
    m_fonts = settings;
    LOG_INFO(QString("Font settings changed: UI=%1 %2pt, Text=%3 %4pt")
             .arg(m_fonts.uiFamily).arg(m_fonts.uiSize)
             .arg(m_fonts.textFamily).arg(m_fonts.textSize));
}

QString ThemeManager::generateStyleSheet() const {
    QString style = QString(R"(
        * {
            font-family: "%1";
            font-size: %2pt;
        }
        QMainWindow, QWidget {
            background-color: %3;
            color: %4;
        }
        QMenuBar {
            background-color: %5;
            color: %4;
        }
        QMenuBar::item:selected {
            background-color: %6;
        }
        QMenu {
            background-color: %5;
            color: %4;
            border: 1px solid %7;
        }
        QMenu::item:selected {
            background-color: %8;
        }
        QToolBar {
            background-color: %5;
            border: none;
            spacing: 5px;
            padding: 5px;
        }
        QToolButton {
            background-color: %6;
            color: %4;
            border: none;
            padding: 5px 10px;
            border-radius: 3px;
        }
        QToolButton:hover {
            background-color: %8;
        }
        QPushButton {
            background-color: %8;
            color: %9;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: %10;
        }
        QPushButton:disabled {
            background-color: %6;
            color: %11;
        }
        QStatusBar {
            background-color: %12;
            color: white;
        }
        QSplitter::handle {
            background-color: %7;
        }
        QTextEdit, QPlainTextEdit {
            background-color: %3;
            color: %4;
            border: 1px solid %7;
            selection-background-color: %13;
            font-family: "%14";
            font-size: %15pt;
        }
        QLineEdit {
            background-color: %3;
            color: %4;
            border: 1px solid %7;
            padding: 4px;
            border-radius: 3px;
        }
        QLineEdit:focus {
            border: 1px solid %8;
        }
        QComboBox {
            background-color: %3;
            color: %4;
            border: 1px solid %7;
            padding: 4px;
            border-radius: 3px;
        }
        QComboBox:hover {
            border: 1px solid %8;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: %5;
            color: %4;
            selection-background-color: %8;
        }
        QSpinBox {
            background-color: %3;
            color: %4;
            border: 1px solid %7;
            padding: 4px;
            border-radius: 3px;
        }
        QSpinBox:focus {
            border: 1px solid %8;
        }
        QScrollBar:vertical {
            background-color: %6;
            width: 14px;
            border-left: 1px solid %7;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background-color: %16;
            border-radius: 5px;
            min-height: 30px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %8;
        }
        QScrollBar::handle:vertical:pressed {
            background-color: %10;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background-color: transparent;
        }
        QScrollBar:horizontal {
            background-color: %6;
            height: 14px;
            border-top: 1px solid %7;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background-color: %16;
            border-radius: 5px;
            min-width: 30px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: %8;
        }
        QScrollBar::handle:horizontal:pressed {
            background-color: %10;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background-color: transparent;
        }
        QTabWidget::pane {
            border: 1px solid %7;
            background-color: %3;
        }
        QTabBar::tab {
            background-color: %5;
            color: %4;
            padding: 8px 16px;
            border: 1px solid %7;
            border-bottom: none;
        }
        QTabBar::tab:selected {
            background-color: %3;
            border-bottom: 2px solid %8;
        }
        QTabBar::tab:hover:!selected {
            background-color: %6;
        }
        QGroupBox {
            border: 1px solid %7;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: %4;
        }
        QListWidget {
            background-color: %3;
            color: %4;
            border: 1px solid %7;
        }
        QListWidget::item:selected {
            background-color: %8;
        }
        QListWidget::item:hover:!selected {
            background-color: %6;
        }
        QLabel {
            color: %4;
        }
        QCheckBox {
            color: %4;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %7;
            border-radius: 3px;
            background-color: %3;
        }
        QCheckBox::indicator:checked {
            background-color: %8;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background-color: %7;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            width: 16px;
            height: 16px;
            background-color: %8;
            border-radius: 8px;
            margin: -6px 0;
        }
        QSlider::handle:horizontal:hover {
            background-color: %10;
        }
        QProgressBar {
            background-color: %6;
            border: none;
            border-radius: 4px;
            text-align: center;
            color: %4;
        }
        QProgressBar::chunk {
            background-color: %8;
            border-radius: 4px;
        }
        QDialog {
            background-color: %3;
        }
    )")
    .arg(m_fonts.uiFamily)           // %1 - UI font family
    .arg(m_fonts.uiSize)             // %2 - UI font size
    .arg(m_colors.background)        // %3 - main background
    .arg(m_colors.text)              // %4 - text color
    .arg(m_colors.backgroundAlt)     // %5 - alt background
    .arg(m_colors.backgroundPanel)   // %6 - panel background
    .arg(m_colors.border)            // %7 - border color
    .arg(m_colors.accent)            // %8 - accent color
    .arg(m_colors.accentText)        // %9 - text on accent
    .arg(m_colors.accentHover)       // %10 - accent hover
    .arg(m_colors.textSecondary)     // %11 - secondary text
    .arg(m_colors.statusBar)         // %12 - status bar
    .arg(m_colors.selection)         // %13 - selection
    .arg(m_fonts.textFamily)         // %14 - text font family
    .arg(m_fonts.textSize)           // %15 - text font size
    .arg(m_colors.scrollHandle);     // %16 - scroll handle

    return style;
}

void ThemeManager::apply() {
    QString styleSheet = generateStyleSheet();
    if (qApp) {
        qApp->setStyleSheet(styleSheet);
        emit themeChanged();
        LOG_INFO("Theme applied to application");
    }
}

void ThemeManager::load() {
    auto& config = Config::instance();

    // Load theme
    m_themeName = config.value("appearance/theme", "dark").toString();
    if (m_themeName == "light") {
        m_colors = lightTheme();
    } else {
        m_colors = darkTheme();
    }

    // Load accent color
    m_accentColor = config.value("appearance/accent_color", "#094771").toString();
    applyAccentColor();

    // Load font settings
    m_fonts.uiFamily = config.value("appearance/font_family", "Segoe UI").toString();
    m_fonts.uiSize = config.value("appearance/font_size", 10).toInt();
    m_fonts.textFamily = config.value("appearance/text_font_family", "Consolas").toString();
    m_fonts.textSize = config.value("appearance/text_font_size", 11).toInt();

    // Load damier settings
    m_damier.enabled = config.value("appearance/damier_enabled", true).toBool();
    m_damier.contrast = config.value("appearance/damier_contrast", 30).toInt();
    applyDamierColors();

    // Load selection colors (with defaults from current theme)
    QString defaultSelBg = m_colors.selection;
    QString defaultSelText = m_colors.selectionText;
    m_colors.selection = config.value("appearance/selection_bg", defaultSelBg).toString();
    m_colors.selectionText = config.value("appearance/selection_text", defaultSelText).toString();

    LOG_INFO(QString("Theme loaded: %1, accent: %2").arg(m_themeName, m_accentColor));
}

void ThemeManager::save() {
    auto& config = Config::instance();

    config.setValue("appearance/theme", m_themeName);
    config.setValue("appearance/accent_color", m_accentColor);
    config.setValue("appearance/font_family", m_fonts.uiFamily);
    config.setValue("appearance/font_size", m_fonts.uiSize);
    config.setValue("appearance/text_font_family", m_fonts.textFamily);
    config.setValue("appearance/text_font_size", m_fonts.textSize);
    config.setValue("appearance/damier_enabled", m_damier.enabled);
    config.setValue("appearance/damier_contrast", m_damier.contrast);
    config.setValue("appearance/selection_bg", m_colors.selection);
    config.setValue("appearance/selection_text", m_colors.selectionText);

    config.save();
    LOG_INFO("Theme settings saved");
}

} // namespace codex::utils
