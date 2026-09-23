#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include "core/utils/backupCrypto.h"

class BackupEncryptionTest : public QObject
{
    Q_OBJECT

private slots:
    void encryptsAndDecryptsBackup();
    void rejectsWrongPassword();
    void rejectsModifiedCiphertext();
};

void BackupEncryptionTest::encryptsAndDecryptsBackup()
{
    const QByteArray plainText = R"({"privateKey":"secret-private-key","server":"vpn.example"})";
    const QString password = QStringLiteral("correct horse battery staple");

    const QByteArray encrypted = BackupCrypto::encrypt(plainText, password);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(BackupCrypto::isEncryptedBackup(encrypted));
    QVERIFY(!encrypted.contains("secret-private-key"));

    QByteArray decrypted;
    QVERIFY(BackupCrypto::decrypt(encrypted, password, decrypted));
    QCOMPARE(decrypted, plainText);
}

void BackupEncryptionTest::rejectsWrongPassword()
{
    const QByteArray encrypted = BackupCrypto::encrypt(
            QByteArrayLiteral(R"({"privateKey":"secret-private-key"})"),
            QStringLiteral("correct horse battery staple"));
    QVERIFY(!encrypted.isEmpty());

    QByteArray decrypted;
    QVERIFY(!BackupCrypto::decrypt(encrypted, QStringLiteral("wrong password"), decrypted));
    QVERIFY(decrypted.isEmpty());
}

void BackupEncryptionTest::rejectsModifiedCiphertext()
{
    const QString password = QStringLiteral("correct horse battery staple");
    const QByteArray encrypted = BackupCrypto::encrypt(
            QByteArrayLiteral(R"({"privateKey":"secret-private-key"})"), password);
    QJsonObject envelope = QJsonDocument::fromJson(encrypted).object();
    QByteArray cipherText = QByteArray::fromBase64(
            envelope.value(QStringLiteral("ciphertext")).toString().toLatin1());
    QVERIFY(!cipherText.isEmpty());
    cipherText[0] = static_cast<char>(cipherText.at(0) ^ 0x01);
    envelope[QStringLiteral("ciphertext")] = QString::fromLatin1(cipherText.toBase64());

    QByteArray decrypted;
    QVERIFY(!BackupCrypto::decrypt(QJsonDocument(envelope).toJson(QJsonDocument::Compact),
                                   password, decrypted));
    QVERIFY(decrypted.isEmpty());
}

QTEST_MAIN(BackupEncryptionTest)
#include "testBackupEncryption.moc"
