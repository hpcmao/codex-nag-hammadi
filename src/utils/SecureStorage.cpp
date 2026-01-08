#include "SecureStorage.h"
#include "Logger.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#endif

#ifdef Q_OS_MACOS
#include <Security/Security.h>
#endif

namespace codex::utils {

SecureStorage& SecureStorage::instance() {
    static SecureStorage instance;
    return instance;
}

SecureStorage::SecureStorage() {
    load();
}

SecureStorage::~SecureStorage() {
    save();
}

QString SecureStorage::storagePath() const {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return appData + "/secure_keys.dat";
}

bool SecureStorage::storeApiKey(const QString& service, const QString& key) {
    if (service.isEmpty() || key.isEmpty()) {
        LOG_ERROR("Cannot store empty service or key");
        return false;
    }

#ifdef Q_OS_MACOS
    // macOS: Use Keychain directly
    if (!storeKeychain(service, key)) {
        return false;
    }
    m_keys[service] = key; // Keep in memory for quick access
    LOG_INFO(QString("API key stored in Keychain for: %1").arg(service));
    return true;
#else
    // Windows/Linux: Store encrypted
    m_keys[service] = key;
    if (!save()) {
        m_keys.remove(service);
        return false;
    }
    LOG_INFO(QString("API key stored securely for: %1").arg(service));
    return true;
#endif
}

QString SecureStorage::getApiKey(const QString& service) const {
#ifdef Q_OS_MACOS
    // Try memory cache first, then Keychain
    if (m_keys.contains(service)) {
        return m_keys[service];
    }
    return retrieveKeychain(service);
#else
    return m_keys.value(service);
#endif
}

bool SecureStorage::hasApiKey(const QString& service) const {
    return !getApiKey(service).isEmpty();
}

bool SecureStorage::removeApiKey(const QString& service) {
#ifdef Q_OS_MACOS
    if (!deleteKeychain(service)) {
        return false;
    }
#endif
    m_keys.remove(service);
    return save();
}

QStringList SecureStorage::listServices() const {
    return m_keys.keys();
}

bool SecureStorage::load() {
    if (m_loaded) {
        return true;
    }

#ifdef Q_OS_MACOS
    // macOS: Keys are in Keychain, just mark as loaded
    m_loaded = true;
    return true;
#endif

    QFile file(storagePath());
    if (!file.exists()) {
        m_loaded = true;
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open secure storage: %1").arg(file.errorString()));
        return false;
    }

    QByteArray encryptedData = file.readAll();
    file.close();

    if (encryptedData.isEmpty()) {
        m_loaded = true;
        return true;
    }

#ifdef Q_OS_WIN
    QByteArray decrypted = decryptWindows(encryptedData);
#else
    QByteArray decrypted = decryptLinux(encryptedData);
#endif

    if (decrypted.isEmpty()) {
        LOG_ERROR("Failed to decrypt secure storage");
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(decrypted);
    if (!doc.isObject()) {
        LOG_ERROR("Invalid secure storage format");
        return false;
    }

    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        m_keys[it.key()] = it.value().toString();
    }

    m_loaded = true;
    LOG_INFO(QString("Loaded %1 API keys from secure storage").arg(m_keys.size()));
    return true;
}

bool SecureStorage::save() {
#ifdef Q_OS_MACOS
    // macOS: Keys are saved directly to Keychain
    return true;
#endif

    QJsonObject obj;
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
        obj[it.key()] = it.value();
    }

    QByteArray jsonData = QJsonDocument(obj).toJson(QJsonDocument::Compact);

#ifdef Q_OS_WIN
    QByteArray encrypted = encryptWindows(jsonData);
#else
    QByteArray encrypted = encryptLinux(jsonData);
#endif

    if (encrypted.isEmpty()) {
        LOG_ERROR("Failed to encrypt secure storage");
        return false;
    }

    QFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to write secure storage: %1").arg(file.errorString()));
        return false;
    }

    file.write(encrypted);
    file.close();

    return true;
}

// ============================================================================
// Windows Implementation (DPAPI)
// ============================================================================
#ifdef Q_OS_WIN

QByteArray SecureStorage::encryptWindows(const QByteArray& data) const {
    DATA_BLOB inputBlob;
    DATA_BLOB outputBlob;

    inputBlob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.data()));
    inputBlob.cbData = static_cast<DWORD>(data.size());

    if (!CryptProtectData(&inputBlob, L"CodexNagHammadi", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &outputBlob)) {
        LOG_ERROR(QString("DPAPI encryption failed: %1").arg(GetLastError()));
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<char*>(outputBlob.pbData),
                      static_cast<int>(outputBlob.cbData));
    LocalFree(outputBlob.pbData);

    return result;
}

QByteArray SecureStorage::decryptWindows(const QByteArray& data) const {
    DATA_BLOB inputBlob;
    DATA_BLOB outputBlob;

    inputBlob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.data()));
    inputBlob.cbData = static_cast<DWORD>(data.size());

    if (!CryptUnprotectData(&inputBlob, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &outputBlob)) {
        LOG_ERROR(QString("DPAPI decryption failed: %1").arg(GetLastError()));
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<char*>(outputBlob.pbData),
                      static_cast<int>(outputBlob.cbData));
    LocalFree(outputBlob.pbData);

    return result;
}

#endif // Q_OS_WIN

// ============================================================================
// macOS Implementation (Keychain)
// ============================================================================
#ifdef Q_OS_MACOS

bool SecureStorage::storeKeychain(const QString& service, const QString& key) {
    QByteArray serviceData = ("com.codex-nag-hammadi." + service).toUtf8();
    QByteArray keyData = key.toUtf8();

    // Delete existing item first
    deleteKeychain(service);

    SecKeychainItemRef item = nullptr;
    OSStatus status = SecKeychainAddGenericPassword(
        nullptr, // default keychain
        static_cast<UInt32>(serviceData.size()),
        serviceData.constData(),
        static_cast<UInt32>(serviceData.size()),
        serviceData.constData(),
        static_cast<UInt32>(keyData.size()),
        keyData.constData(),
        &item
    );

    if (item) {
        CFRelease(item);
    }

    if (status != errSecSuccess) {
        LOG_ERROR(QString("Keychain store failed: %1").arg(status));
        return false;
    }

    return true;
}

QString SecureStorage::retrieveKeychain(const QString& service) const {
    QByteArray serviceData = ("com.codex-nag-hammadi." + service).toUtf8();

    void* passwordData = nullptr;
    UInt32 passwordLength = 0;

    OSStatus status = SecKeychainFindGenericPassword(
        nullptr, // default keychain
        static_cast<UInt32>(serviceData.size()),
        serviceData.constData(),
        static_cast<UInt32>(serviceData.size()),
        serviceData.constData(),
        &passwordLength,
        &passwordData,
        nullptr
    );

    if (status != errSecSuccess) {
        if (status != errSecItemNotFound) {
            LOG_ERROR(QString("Keychain retrieve failed: %1").arg(status));
        }
        return QString();
    }

    QString result = QString::fromUtf8(
        static_cast<char*>(passwordData),
        static_cast<int>(passwordLength)
    );

    SecKeychainItemFreeContent(nullptr, passwordData);

    return result;
}

bool SecureStorage::deleteKeychain(const QString& service) {
    QByteArray serviceData = ("com.codex-nag-hammadi." + service).toUtf8();

    SecKeychainItemRef item = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        static_cast<UInt32>(serviceData.size()),
        serviceData.constData(),
        static_cast<UInt32>(serviceData.size()),
        serviceData.constData(),
        nullptr,
        nullptr,
        &item
    );

    if (status == errSecSuccess && item) {
        status = SecKeychainItemDelete(item);
        CFRelease(item);
    }

    return status == errSecSuccess || status == errSecItemNotFound;
}

#endif // Q_OS_MACOS

// ============================================================================
// Linux Implementation (Encrypted file with derived key)
// ============================================================================
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)

QByteArray SecureStorage::deriveKey() const {
    // Derive a key from machine-specific data
    // This is not as secure as DPAPI or Keychain but provides basic protection
    QString machineId;

    // Try to read machine-id
    QFile machineIdFile("/etc/machine-id");
    if (machineIdFile.open(QIODevice::ReadOnly)) {
        machineId = QString::fromUtf8(machineIdFile.readAll()).trimmed();
        machineIdFile.close();
    }

    if (machineId.isEmpty()) {
        // Fallback to hostname
        machineId = QSysInfo::machineHostName();
    }

    // Add app-specific salt
    QString keyMaterial = machineId + "::CodexNagHammadi::SecureStorage";

    return QCryptographicHash::hash(keyMaterial.toUtf8(), QCryptographicHash::Sha256);
}

QByteArray SecureStorage::encryptLinux(const QByteArray& data) const {
    // Simple XOR encryption with derived key
    // For production, consider using OpenSSL or libsodium
    QByteArray key = deriveKey();
    QByteArray result = data;

    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % key.size()];
    }

    // Add a simple marker to verify decryption
    QByteArray marker = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    return marker + result;
}

QByteArray SecureStorage::decryptLinux(const QByteArray& data) const {
    if (data.size() < 16) { // MD5 hash size
        return QByteArray();
    }

    QByteArray marker = data.left(16);
    QByteArray encrypted = data.mid(16);
    QByteArray key = deriveKey();

    QByteArray result = encrypted;
    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % key.size()];
    }

    // Verify marker
    QByteArray expectedMarker = QCryptographicHash::hash(result, QCryptographicHash::Md5);
    if (marker != expectedMarker) {
        LOG_ERROR("Secure storage integrity check failed");
        return QByteArray();
    }

    return result;
}

#endif // Linux

} // namespace codex::utils
