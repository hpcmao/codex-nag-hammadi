#pragma once

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QTemporaryFile>

namespace codex::api {

struct EdgeVoiceSettings {
    QString voiceId = "fr-FR-HenriNeural";  // Neural voice name
    int rate = 0;       // Speed: -100 to 100 (percent)
    int pitch = 0;      // Pitch: -100 to 100 (Hz adjustment)
    int volume = 100;   // Volume: 0 to 100
};

class EdgeTTSClient : public QObject {
    Q_OBJECT

public:
    explicit EdgeTTSClient(QObject* parent = nullptr);
    ~EdgeTTSClient();

    // Generate speech from text (returns MP3 audio data)
    void generateSpeech(const QString& text, const EdgeVoiceSettings& settings = EdgeVoiceSettings());

    // Stop current generation
    void stop();

    // Check if generation is in progress
    bool isGenerating() const { return m_isGenerating; }

    // Available French neural voices
    static QStringList availableVoices();

signals:
    void speechGenerated(const QByteArray& audioData, int durationMs);
    void generationProgress(int percent);
    void errorOccurred(const QString& error);
    void requestStarted();
    void requestFinished();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    int estimateDuration(const QString& text);

    QProcess* m_process = nullptr;
    QString m_outputFilePath;
    QString m_textFilePath;
    QString m_currentText;
    bool m_isGenerating = false;
};

} // namespace codex::api
