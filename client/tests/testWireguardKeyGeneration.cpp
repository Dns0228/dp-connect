#include <QByteArray>
#include <QTest>

#include <openssl/evp.h>

#include "core/configurators/wireguardConfigurator.h"

class TestWireguardKeyGeneration : public QObject
{
    Q_OBJECT

private slots:
    void createsIndependentPerPeerSecrets()
    {
        const auto first = WireguardConfigurator::genClientKeys();
        const auto second = WireguardConfigurator::genClientKeys();

        const QByteArray firstPrivate = QByteArray::fromBase64(first.clientPrivKey.toLatin1());
        const QByteArray firstPublic = QByteArray::fromBase64(first.clientPubKey.toLatin1());
        const QByteArray firstPsk = QByteArray::fromBase64(first.pskKey.toLatin1());
        const QByteArray secondPrivate = QByteArray::fromBase64(second.clientPrivKey.toLatin1());
        const QByteArray secondPsk = QByteArray::fromBase64(second.pskKey.toLatin1());

        QCOMPARE(firstPrivate.size(), 32);
        QCOMPARE(firstPublic.size(), 32);
        QCOMPARE(firstPsk.size(), 32);
        QCOMPARE(secondPrivate.size(), 32);
        QCOMPARE(secondPsk.size(), 32);
        QVERIFY(firstPrivate != secondPrivate);
        QVERIFY(firstPsk != secondPsk);
        QVERIFY(firstPrivate != firstPsk);
    }

    void publicKeyMatchesPrivateKey()
    {
        const auto keys = WireguardConfigurator::genClientKeys();
        const QByteArray privateKey = QByteArray::fromBase64(keys.clientPrivKey.toLatin1());
        const QByteArray expectedPublicKey = QByteArray::fromBase64(keys.clientPubKey.toLatin1());
        QCOMPARE(privateKey.size(), 32);
        QCOMPARE(expectedPublicKey.size(), 32);

        EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(
                EVP_PKEY_X25519, nullptr,
                reinterpret_cast<const unsigned char *>(privateKey.constData()),
                static_cast<size_t>(privateKey.size()));
        QVERIFY(pkey != nullptr);

        unsigned char publicKey[32] = {};
        size_t publicKeyLength = sizeof(publicKey);
        QCOMPARE(EVP_PKEY_get_raw_public_key(pkey, publicKey, &publicKeyLength), 1);
        EVP_PKEY_free(pkey);

        QCOMPARE(publicKeyLength, size_t(32));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(publicKey), 32), expectedPublicKey);
    }
};

QTEST_MAIN(TestWireguardKeyGeneration)
#include "testWireguardKeyGeneration.moc"
