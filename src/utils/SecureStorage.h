#pragma once

#include <QString>
#include <QMap>

namespace codex::utils {

/**
 * SecureStorage - Stockage sécurisé des clés API
 *
 * Utilise les mécanismes natifs de chaque plateforme:
 * - Windows: DPAPI (Data Protection API)
 * - macOS: Keychain Services
 * - Linux: Fichier chiffré avec clé dérivée
 */
class SecureStorage {
public:
    static SecureStorage& instance();

    // Stocke une clé API de manière sécurisée
    bool storeApiKey(const QString& service, const QString& key);

    // Récupère une clé API
    QString getApiKey(const QString& service) const;

    // Vérifie si une clé existe
    bool hasApiKey(const QString& service) const;

    // Supprime une clé
    bool removeApiKey(const QString& service);

    // Liste les services stockés
    QStringList listServices() const;

    // Services prédéfinis
    static constexpr const char* SERVICE_CLAUDE = "claude_api";
    static constexpr const char* SERVICE_IMAGEN = "imagen_api";
    static constexpr const char* SERVICE_ELEVENLABS = "elevenlabs_api";

private:
    SecureStorage();
    ~SecureStorage();
    SecureStorage(const SecureStorage&) = delete;
    SecureStorage& operator=(const SecureStorage&) = delete;

    // Chemin du fichier de stockage
    QString storagePath() const;

    // Chargement/sauvegarde
    bool load();
    bool save();

#ifdef Q_OS_WIN
    // Windows: DPAPI
    QByteArray encryptWindows(const QByteArray& data) const;
    QByteArray decryptWindows(const QByteArray& data) const;
#elif defined(Q_OS_MACOS)
    // macOS: Keychain
    bool storeKeychain(const QString& service, const QString& key);
    QString retrieveKeychain(const QString& service) const;
    bool deleteKeychain(const QString& service);
#else
    // Linux: Fichier chiffré
    QByteArray deriveKey() const;
    QByteArray encryptLinux(const QByteArray& data) const;
    QByteArray decryptLinux(const QByteArray& data) const;
#endif

    QMap<QString, QString> m_keys;
    bool m_loaded = false;
};

} // namespace codex::utils
