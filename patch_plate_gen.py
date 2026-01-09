# Patch PassagePreviewWidget.h to add plate generation
filepath_h = r"C:\Users\hpcmao\Documents\_Programmation\_codex-nag-hammadi\src\ui\widgets\PassagePreviewWidget.h"
with open(filepath_h, 'r', encoding='utf-8') as f:
    content_h = f.read()

# Add QComboBox include
if 'QComboBox' not in content_h:
    content_h = content_h.replace('#include <QPushButton>', '#include <QPushButton>\n#include <QComboBox>')
    print("Added QComboBox include")

# Add signal for plate generation
old_signals = '''signals:
    void generateImageRequested(const QString& passage);
    void generateAudioRequested(const QString& passage);
    void generateVideoRequested(const QString& passage);
    void favoriteChanged();'''

new_signals = '''signals:
    void generateImageRequested(const QString& passage);
    void generateAudioRequested(const QString& passage);
    void generateVideoRequested(const QString& passage);
    void generatePlateRequested(const QString& passage, int cols, int rows);
    void favoriteChanged();'''

if 'generatePlateRequested' not in content_h:
    content_h = content_h.replace(old_signals, new_signals)
    print("Added generatePlateRequested signal")

# Add slot
old_slots = '''private slots:
    void onGenerateImage();
    void onGenerateAudio();
    void onGenerateVideo();
    void onToggleStar();
    void onToggleHeart();'''

new_slots = '''private slots:
    void onGenerateImage();
    void onGenerateAudio();
    void onGenerateVideo();
    void onGeneratePlate();
    void onToggleStar();
    void onToggleHeart();'''

if 'onGeneratePlate' not in content_h:
    content_h = content_h.replace(old_slots, new_slots)
    print("Added onGeneratePlate slot")

# Add member variables
old_members = '''    QPushButton* m_starBtn;
    QPushButton* m_heartBtn;

    QString m_passage;'''

new_members = '''    QPushButton* m_starBtn;
    QPushButton* m_heartBtn;
    QComboBox* m_plateSizeCombo = nullptr;
    QPushButton* m_generatePlateBtn = nullptr;

    QString m_passage;'''

if 'm_plateSizeCombo' not in content_h:
    content_h = content_h.replace(old_members, new_members)
    print("Added plate member variables")

with open(filepath_h, 'w', encoding='utf-8') as f:
    f.write(content_h)

print("PassagePreviewWidget.h patched")

# Now patch the .cpp file
filepath_cpp = r"C:\Users\hpcmao\Documents\_Programmation\_codex-nag-hammadi\src\ui\widgets\PassagePreviewWidget.cpp"
with open(filepath_cpp, 'r', encoding='utf-8') as f:
    content_cpp = f.read()

# Add plate generation UI after the button layout
old_buttons = '''    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Set fixed height for the entire widget
    setMaximumHeight(250);
}'''

new_buttons = '''    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Plate generation row
    auto* plateGroup = new QGroupBox("Generer une Planche", this);
    plateGroup->setStyleSheet(R"(
        QGroupBox {
            color: #d4d4d4;
            font-weight: bold;
            border: 1px solid #3d3d3d;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    )");
    auto* plateLayout = new QHBoxLayout(plateGroup);
    plateLayout->setSpacing(10);

    plateLayout->addWidget(new QLabel("Taille:", this));

    m_plateSizeCombo = new QComboBox(this);
    m_plateSizeCombo->addItem("2x2 (4)", QSize(2, 2));
    m_plateSizeCombo->addItem("3x2 (6)", QSize(3, 2));
    m_plateSizeCombo->addItem("3x3 (9)", QSize(3, 3));
    m_plateSizeCombo->addItem("3x4 (12)", QSize(3, 4));
    m_plateSizeCombo->addItem("4x4 (16)", QSize(4, 4));
    m_plateSizeCombo->addItem("4x5 (20)", QSize(4, 5));
    m_plateSizeCombo->addItem("5x6 (30)", QSize(5, 6));
    m_plateSizeCombo->setMinimumWidth(100);
    plateLayout->addWidget(m_plateSizeCombo);

    m_generatePlateBtn = new QPushButton("Generer Planche", this);
    m_generatePlateBtn->setEnabled(false);
    m_generatePlateBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2d5a27;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 3px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #3d7a37;
        }
        QPushButton:disabled {
            background-color: #3d3d3d;
            color: #666;
        }
    )");
    connect(m_generatePlateBtn, &QPushButton::clicked, this, &PassagePreviewWidget::onGeneratePlate);
    plateLayout->addWidget(m_generatePlateBtn);

    plateLayout->addStretch();
    mainLayout->addWidget(plateGroup);

    // Set fixed height for the entire widget
    setMaximumHeight(320);
}'''

if 'Generer une Planche' not in content_cpp:
    content_cpp = content_cpp.replace(old_buttons, new_buttons)
    print("Added plate generation UI")

# Update setPassage to enable plate button
old_enable = '''    m_generateImageBtn->setEnabled(valid);
    m_generateAudioBtn->setEnabled(valid);
    m_generateVideoBtn->setEnabled(valid);'''

new_enable = '''    m_generateImageBtn->setEnabled(valid);
    m_generateAudioBtn->setEnabled(valid);
    m_generateVideoBtn->setEnabled(valid);
    m_generatePlateBtn->setEnabled(valid);'''

if 'm_generatePlateBtn->setEnabled' not in content_cpp:
    content_cpp = content_cpp.replace(old_enable, new_enable)
    print("Added plate button enable")

# Update clear to disable plate button
old_clear = '''    m_generateImageBtn->setEnabled(false);
    m_generateAudioBtn->setEnabled(false);
    m_generateVideoBtn->setEnabled(false);'''

new_clear = '''    m_generateImageBtn->setEnabled(false);
    m_generateAudioBtn->setEnabled(false);
    m_generateVideoBtn->setEnabled(false);
    m_generatePlateBtn->setEnabled(false);'''

if 'm_generatePlateBtn->setEnabled(false)' not in content_cpp:
    content_cpp = content_cpp.replace(old_clear, new_clear)
    print("Added plate button disable in clear")

# Add onGeneratePlate implementation
old_gen_video = '''void PassagePreviewWidget::onGenerateVideo() {
    if (!m_passage.isEmpty()) {
        emit generateVideoRequested(m_passage);
    }
}'''

new_gen_video = '''void PassagePreviewWidget::onGenerateVideo() {
    if (!m_passage.isEmpty()) {
        emit generateVideoRequested(m_passage);
    }
}

void PassagePreviewWidget::onGeneratePlate() {
    if (!m_passage.isEmpty() && m_plateSizeCombo) {
        QSize size = m_plateSizeCombo->currentData().toSize();
        emit generatePlateRequested(m_passage, size.width(), size.height());
        LOG_INFO(QString("Plate generation requested: %1x%2").arg(size.width()).arg(size.height()));
    }
}'''

if 'onGeneratePlate()' not in content_cpp:
    content_cpp = content_cpp.replace(old_gen_video, new_gen_video)
    print("Added onGeneratePlate implementation")

with open(filepath_cpp, 'w', encoding='utf-8') as f:
    f.write(content_cpp)

print("PassagePreviewWidget.cpp patched")
print("Done!")
