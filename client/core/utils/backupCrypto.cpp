#include "backupCrypto.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <limits>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace
{
constexpr int SaltLength = 16;
constexpr int NonceLength = 12;
constexpr int KeyLength = 32;
constexpr int TagLength = 16;
constexpr int KdfIterations = 600000;
constexpr char FormatName[] = "DP_CONNECT_BACKUP";
constexpr char CipherName[] = "AES-256-GCM";
constexpr char KdfName[] = "PBKDF2-HMAC-SHA256";
constexpr char AssociatedData[] = "DP_CONNECT_BACKUP_V1";
constexpr int AssociatedDataLength = static_cast<int>(sizeof(AssociatedData) - 1);

bool fitsOpenSslInt(qsizetype size)
{
    return size >= 0 && size <= std::numeric_limits<int>::max();
}

bool deriveKey(const QString &passphrase, const QByteArray &salt, QByteArray &key)
{
    QByteArray password = passphrase.toUtf8();
    if (!fitsOpenSslInt(password.size())) {
        OPENSSL_cleanse(password.data(), static_cast<size_t>(password.size()));
        return false;
    }
    key.resize(KeyLength);
    const int result = PKCS5_PBKDF2_HMAC(
            password.constData(), static_cast<int>(password.size()),
            reinterpret_cast<const unsigned char *>(salt.constData()), static_cast<int>(salt.size()),
            KdfIterations, EVP_sha256(), static_cast<int>(key.size()),
            reinterpret_cast<unsigned char *>(key.data()));
    OPENSSL_cleanse(password.data(), static_cast<size_t>(password.size()));
    if (result != 1) {
        OPENSSL_cleanse(key.data(), static_cast<size_t>(key.size()));
        key.clear();
        return false;
    }
    return true;
}

QByteArray decodeField(const QJsonObject &object, const char *name)
{
    return QByteArray::fromBase64(object.value(QLatin1String(name)).toString().toLatin1(),
                                  QByteArray::AbortOnBase64DecodingErrors);
}
}

QByteArray BackupCrypto::encrypt(const QByteArray &plainText, const QString &passphrase)
{
    if (plainText.isEmpty() || passphrase.isEmpty()
        || !fitsOpenSslInt(plainText.size())) {
        return {};
    }

    QByteArray salt(SaltLength, Qt::Uninitialized);
    QByteArray nonce(NonceLength, Qt::Uninitialized);
    if (RAND_priv_bytes(reinterpret_cast<unsigned char *>(salt.data()), static_cast<int>(salt.size())) != 1
        || RAND_priv_bytes(reinterpret_cast<unsigned char *>(nonce.data()), static_cast<int>(nonce.size())) != 1) {
        return {};
    }

    QByteArray key;
    if (!deriveKey(passphrase, salt, key)) return {};

    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new();
    QByteArray cipherText(plainText.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    QByteArray tag(TagLength, Qt::Uninitialized);
    int written = 0;
    int total = 0;
    int aadWritten = 0;
    bool ok = context
            && EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
            && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN,
                                   static_cast<int>(nonce.size()), nullptr) == 1
            && EVP_EncryptInit_ex(context, nullptr, nullptr,
                                  reinterpret_cast<const unsigned char *>(key.constData()),
                                  reinterpret_cast<const unsigned char *>(nonce.constData())) == 1
            && EVP_EncryptUpdate(context, nullptr, &aadWritten,
                                 reinterpret_cast<const unsigned char *>(AssociatedData),
                                 AssociatedDataLength) == 1
            && EVP_EncryptUpdate(context,
                                 reinterpret_cast<unsigned char *>(cipherText.data()), &written,
                                 reinterpret_cast<const unsigned char *>(plainText.constData()),
                                 static_cast<int>(plainText.size())) == 1;
    total = written;
    if (ok) {
        ok = EVP_EncryptFinal_ex(context,
                                 reinterpret_cast<unsigned char *>(cipherText.data()) + total,
                                 &written) == 1;
        total += written;
    }
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG,
                                 static_cast<int>(tag.size()), tag.data()) == 1;
    }
    EVP_CIPHER_CTX_free(context);
    OPENSSL_cleanse(key.data(), static_cast<size_t>(key.size()));
    if (!ok) return {};

    cipherText.resize(total);
    QJsonObject envelope;
    envelope[QStringLiteral("format")] = QLatin1String(FormatName);
    envelope[QStringLiteral("version")] = 1;
    envelope[QStringLiteral("cipher")] = QLatin1String(CipherName);
    envelope[QStringLiteral("kdf")] = QLatin1String(KdfName);
    envelope[QStringLiteral("iterations")] = KdfIterations;
    envelope[QStringLiteral("salt")] = QString::fromLatin1(salt.toBase64());
    envelope[QStringLiteral("nonce")] = QString::fromLatin1(nonce.toBase64());
    envelope[QStringLiteral("tag")] = QString::fromLatin1(tag.toBase64());
    envelope[QStringLiteral("ciphertext")] = QString::fromLatin1(cipherText.toBase64());
    return QJsonDocument(envelope).toJson(QJsonDocument::Compact);
}

bool BackupCrypto::decrypt(const QByteArray &backupData, const QString &passphrase, QByteArray &plainText)
{
    plainText.clear();
    QJsonParseError parseError;
    const QJsonObject envelope = QJsonDocument::fromJson(backupData, &parseError).object();
    if (parseError.error != QJsonParseError::NoError
        || envelope.value(QStringLiteral("format")).toString() != QLatin1String(FormatName)
        || envelope.value(QStringLiteral("version")).toInt() != 1
        || envelope.value(QStringLiteral("cipher")).toString() != QLatin1String(CipherName)
        || envelope.value(QStringLiteral("kdf")).toString() != QLatin1String(KdfName)
        || envelope.value(QStringLiteral("iterations")).toInt() != KdfIterations
        || passphrase.isEmpty()) {
        return false;
    }

    const QByteArray salt = decodeField(envelope, "salt");
    const QByteArray nonce = decodeField(envelope, "nonce");
    const QByteArray tag = decodeField(envelope, "tag");
    const QByteArray cipherText = decodeField(envelope, "ciphertext");
    if (salt.size() != SaltLength || nonce.size() != NonceLength || tag.size() != TagLength
        || cipherText.isEmpty() || !fitsOpenSslInt(cipherText.size())) {
        return false;
    }

    QByteArray key;
    if (!deriveKey(passphrase, salt, key)) return false;

    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new();
    QByteArray decrypted(cipherText.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int written = 0;
    int total = 0;
    int aadWritten = 0;
    bool ok = context
            && EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
            && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN,
                                   static_cast<int>(nonce.size()), nullptr) == 1
            && EVP_DecryptInit_ex(context, nullptr, nullptr,
                                  reinterpret_cast<const unsigned char *>(key.constData()),
                                  reinterpret_cast<const unsigned char *>(nonce.constData())) == 1
            && EVP_DecryptUpdate(context, nullptr, &aadWritten,
                                 reinterpret_cast<const unsigned char *>(AssociatedData),
                                 AssociatedDataLength) == 1
            && EVP_DecryptUpdate(context,
                                 reinterpret_cast<unsigned char *>(decrypted.data()), &written,
                                 reinterpret_cast<const unsigned char *>(cipherText.constData()),
                                 static_cast<int>(cipherText.size())) == 1;
    total = written;
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()),
                                 const_cast<char *>(tag.constData())) == 1
                && EVP_DecryptFinal_ex(context,
                                       reinterpret_cast<unsigned char *>(decrypted.data()) + total,
                                       &written) == 1;
        total += written;
    }
    EVP_CIPHER_CTX_free(context);
    OPENSSL_cleanse(key.data(), static_cast<size_t>(key.size()));
    if (!ok) {
        OPENSSL_cleanse(decrypted.data(), static_cast<size_t>(decrypted.size()));
        return false;
    }

    decrypted.resize(total);
    plainText = decrypted;
    return true;
}

bool BackupCrypto::isEncryptedBackup(const QByteArray &backupData)
{
    const QJsonObject object = QJsonDocument::fromJson(backupData).object();
    return object.value(QStringLiteral("format")).toString() == QLatin1String(FormatName);
}
