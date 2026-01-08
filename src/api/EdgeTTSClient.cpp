#include "EdgeTTSClient.h"
#include "utils/Logger.h"

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>

namespace codex::api {

EdgeTTSClient::EdgeTTSClient(QObject* parent)
    : QObject(parent)
{
    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &EdgeTTSClient::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &EdgeTTSClient::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &EdgeTTSClient::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &EdgeTTSClient::onReadyReadStandardError);

    LOG_INFO("EdgeTTS: Initialized (using Python edge-tts for Neural voices)");
}

EdgeTTSClient::~EdgeTTSClient() {
    stop();
    // Clean up temp file
    if (!m_outputFilePath.isEmpty() && QFile::exists(m_outputFilePath)) {
        QFile::remove(m_outputFilePath);
    }
}

QStringList EdgeTTSClient::availableVoices() {
    return QStringList{
        "fr-FR-HenriNeural",
        "fr-FR-DeniseNeural",
        "fr-FR-EloiseNeural",
        "fr-FR-RemyMultilingualNeural",
        "fr-CA-AntoineNeural",
        "fr-CA-SylvieNeural",
        "en-US-GuyNeural",
        "en-US-JennyNeural",
        "en-US-AriaNeural",
        "en-GB-RyanNeural",
        "en-GB-SoniaNeural"
    };
}

void EdgeTTSClient::stop() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    m_isGenerating = false;
}

void EdgeTTSClient::generateSpeech(const QString& text, const EdgeVoiceSettings& settings) {
    if (m_isGenerating) {
        emit errorOccurred("Une generation est deja en cours");
        return;
    }

    if (text.isEmpty()) {
        emit errorOccurred("Le texte est vide");
        return;
    }

    m_isGenerating = true;
    m_currentText = text;

    emit requestStarted();
    emit generationProgress(5);

    // Clean up previous temp file
    if (!m_outputFilePath.isEmpty() && QFile::exists(m_outputFilePath)) {
        QFile::remove(m_outputFilePath);
    }

    // Create temp file path for output
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_outputFilePath = QString("%1/edge_tts_%2.mp3")
        .arg(tempDir)
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));

    // Build edge-tts command arguments
    QStringList args;
    args << "-v" << settings.voiceId;

    // Rate: convert from -100..100 to edge-tts format (+X% or -X%)
    if (settings.rate != 0) {
        QString rateStr = settings.rate > 0
            ? QString("+%1%").arg(settings.rate)
            : QString("%1%").arg(settings.rate);
        args << "--rate" << rateStr;
    }

    // Pitch: convert to Hz format
    if (settings.pitch != 0) {
        QString pitchStr = settings.pitch > 0
            ? QString("+%1Hz").arg(settings.pitch)
            : QString("%1Hz").arg(settings.pitch);
        args << "--pitch" << pitchStr;
    }

    // Volume: edge-tts uses +X% or -X%
    if (settings.volume != 100) {
        int volumeAdjust = settings.volume - 100;
        QString volStr = volumeAdjust > 0
            ? QString("+%1%").arg(volumeAdjust)
            : QString("%1%").arg(volumeAdjust);
        args << "--volume" << volStr;
    }

    // Text and output file
    args << "--text" << text;
    args << "--write-media" << m_outputFilePath;

    LOG_INFO(QString("EdgeTTS: Starting generation, voice: %1, text length: %2")
             .arg(settings.voiceId).arg(text.length()));
    LOG_INFO(QString("EdgeTTS: Output file: %1").arg(m_outputFilePath));

    emit generationProgress(10);

    // Start edge-tts process
    m_process->start("edge-tts", args);
}

void EdgeTTSClient::onReadyReadStandardOutput() {
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    // Edge-tts outputs progress info here
    if (!output.isEmpty()) {
        LOG_INFO(QString("EdgeTTS: %1").arg(output.trimmed()));
    }
    emit generationProgress(50);
}

void EdgeTTSClient::onReadyReadStandardError() {
    QString error = QString::fromUtf8(m_process->readAllStandardError());
    if (!error.isEmpty()) {
        LOG_INFO(QString("EdgeTTS stderr: %1").arg(error.trimmed()));
    }
}

void EdgeTTSClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_isGenerating = false;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        // Read the generated MP3 file
        QFile audioFile(m_outputFilePath);
        if (audioFile.open(QIODevice::ReadOnly)) {
            QByteArray audioData = audioFile.readAll();
            audioFile.close();

            if (!audioData.isEmpty()) {
                emit generationProgress(100);
                emit requestFinished();

                int durationMs = estimateDuration(m_currentText);
                emit speechGenerated(audioData, durationMs);

                LOG_INFO(QString("EdgeTTS: Generation complete, audio size: %1 bytes")
                         .arg(audioData.size()));
            } else {
                emit requestFinished();
                emit errorOccurred("Le fichier audio genere est vide");
                LOG_ERROR("EdgeTTS: Generated audio file is empty");
            }
        } else {
            emit requestFinished();
            emit errorOccurred(QString("Impossible de lire le fichier audio: %1").arg(m_outputFilePath));
            LOG_ERROR(QString("EdgeTTS: Could not open audio file: %1").arg(m_outputFilePath));
        }
    } else {
        QString errorOutput = m_process->readAllStandardError();
        emit requestFinished();
        emit errorOccurred(QString("Erreur edge-tts (code %1): %2").arg(exitCode).arg(errorOutput));
        LOG_ERROR(QString("EdgeTTS: Process failed with code %1: %2").arg(exitCode).arg(errorOutput));
    }
}

void EdgeTTSClient::onProcessError(QProcess::ProcessError error) {
    m_isGenerating = false;

    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "edge-tts n'a pas pu demarrer. Verifiez que Python et edge-tts sont installes (pip install edge-tts)";
            break;
        case QProcess::Crashed:
            errorMsg = "edge-tts a plante";
            break;
        case QProcess::Timedout:
            errorMsg = "Timeout edge-tts";
            break;
        default:
            errorMsg = "Erreur edge-tts inconnue";
            break;
    }

    emit requestFinished();
    emit errorOccurred(errorMsg);
    LOG_ERROR(QString("EdgeTTS: Process error - %1").arg(errorMsg));
}

int EdgeTTSClient::estimateDuration(const QString& text) {
    // Estimate duration based on text length
    // Average speaking rate: ~150 words per minute, ~5 chars per word
    int wordCount = text.length() / 5;
    return (wordCount * 60 * 1000) / 150;
}

} // namespace codex::api
