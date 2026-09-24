#include <QByteArray>
#include <QRegularExpression>
#include <QSet>
#include <QTest>

#include <openssl/evp.h>

#include "core/configurators/wireguardConfigurator.h"
#include "core/installers/awgInstaller.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

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

    void createsDiversifiedAwg3Profiles()
    {
        AwgInstaller installer;
        QSet<QString> profileFingerprints;
        const QRegularExpression randomPacketPattern(QStringLiteral("^<r ([0-9]+)>$"));

        for (int sample = 0; sample < 12; ++sample) {
            ContainerConfig config = installer.generateConfig(DockerContainer::Awg2, 55424, TransportProto::Udp);
            const AwgProtocolConfig *awg = config.getAwgProtocolConfig();
            QVERIFY(awg != nullptr);

            const AwgServerConfig &server = awg->serverConfig;
            QCOMPARE(server.protocolVersion, QString(protocols::awg::awgV3));

            const int jc = server.junkPacketCount.toInt();
            const int jmin = server.junkPacketMinSize.toInt();
            const int jmax = server.junkPacketMaxSize.toInt();
            QVERIFY(jc >= 4 && jc <= 12);
            QVERIFY(jmin >= 8 && jmin <= 24);
            QVERIFY(jmax > jmin && jmax <= 96);

            const int s1 = server.initPacketJunkSize.toInt();
            const int s2 = server.responsePacketJunkSize.toInt();
            const int s3 = server.cookieReplyPacketJunkSize.toInt();
            const int s4 = server.transportPacketJunkSize.toInt();
            QVERIFY(s1 >= 12 && s1 <= protocols::awg::initPacketJunkSizeMax);
            QVERIFY(s2 >= 12 && s2 <= protocols::awg::responsePacketJunkSizeMax);
            QVERIFY(s3 >= 12 && s3 <= protocols::awg::cookieReplyPacketJunkSizeMax);
            QVERIFY(s4 >= 12 && s4 <= protocols::awg::cookieReplyPacketJunkSizeMax);

            const QSet<int> packetSizes {
                AwgConstant::messageInitiationSize + s1,
                AwgConstant::messageResponseSize + s2,
                AwgConstant::messageCookieReplySize + s3,
                AwgConstant::messageTransportSize + s4
            };
            QCOMPARE(packetSizes.size(), 4);

            const QStringList headerStrings {
                server.initPacketMagicHeader,
                server.responsePacketMagicHeader,
                server.underloadPacketMagicHeader,
                server.transportPacketMagicHeader
            };
            QSet<quint32> headers;
            for (const QString &headerString : headerStrings) {
                bool ok = false;
                const quint32 header = headerString.toUInt(&ok);
                QVERIFY(ok);
                QVERIFY(header >= 5 && header <= 0x7fffffffU);
                headers.insert(header);
            }
            QCOMPARE(headers.size(), 4);

            QCOMPARE(QByteArray::fromBase64(server.headerProtectionKey.toLatin1()).size(), 32);
            const QStringList contentPadding = server.contentPaddingAddition.split('-');
            QCOMPARE(contentPadding.size(), 2);
            QVERIFY(contentPadding.at(0).toInt() >= 10);
            QVERIFY(contentPadding.at(1).toInt() <= 150);
            QVERIFY(contentPadding.at(0).toInt() < contentPadding.at(1).toInt());

            const QRegularExpressionMatch firstPacket = randomPacketPattern.match(server.specialJunk1);
            QVERIFY(firstPacket.hasMatch());
            const int firstPacketSize = firstPacket.captured(1).toInt();
            QVERIFY(firstPacketSize >= 32 && firstPacketSize <= 96);
            if (!server.specialJunk2.isEmpty()) {
                const QRegularExpressionMatch secondPacket = randomPacketPattern.match(server.specialJunk2);
                QVERIFY(secondPacket.hasMatch());
                const int secondPacketSize = secondPacket.captured(1).toInt();
                QVERIFY(secondPacketSize >= 24 && secondPacketSize <= 80);
            }

            profileFingerprints.insert(QStringList {
                server.junkPacketCount,
                server.junkPacketMinSize,
                server.junkPacketMaxSize,
                server.initPacketJunkSize,
                server.responsePacketJunkSize,
                server.cookieReplyPacketJunkSize,
                server.transportPacketJunkSize,
                server.initPacketMagicHeader,
                server.responsePacketMagicHeader,
                server.underloadPacketMagicHeader,
                server.transportPacketMagicHeader,
                server.contentPaddingAddition,
                server.specialJunk1,
                server.specialJunk2
            }.join('|'));
        }

        QVERIFY(profileFingerprints.size() > 1);
    }
};

QTEST_MAIN(TestWireguardKeyGeneration)
#include "testWireguardKeyGeneration.moc"
