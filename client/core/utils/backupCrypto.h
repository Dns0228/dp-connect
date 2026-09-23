#ifndef BACKUPCRYPTO_H
#define BACKUPCRYPTO_H

#include <QByteArray>
#include <QString>

namespace BackupCrypto
{
QByteArray encrypt(const QByteArray &plainText, const QString &passphrase);
bool decrypt(const QByteArray &backupData, const QString &passphrase, QByteArray &plainText);
bool isEncryptedBackup(const QByteArray &backupData);
}

#endif // BACKUPCRYPTO_H
