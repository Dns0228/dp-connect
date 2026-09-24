#include "awgInstaller.h"

#include <QRandomGenerator>
#include <QSet>
#include <QStringList>

#include "core/configurators/wireguardConfigurator.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/utilities.h"
#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

AwgInstaller::AwgInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ContainerConfig AwgInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    ContainerConfig config = createBaseConfig(container, port, transportProto);
    if (auto* awgConfig = config.getAwgProtocolConfig()) {
        generateAwgParameters(awgConfig->serverConfig);
        awgConfig->serverConfig.protocolVersion = protocols::awg::awgV3;
    }
    return config;
}

void AwgInstaller::generateAwgParameters(AwgServerConfig &serverConfig)
{
    QRandomGenerator *random = QRandomGenerator::system();
    const auto boundedInclusive = [random](int minimum, int maximum) {
        return random->bounded(minimum, maximum + 1);
    };

    const int junkPacketCount = boundedInclusive(4, 12);
    const int junkPacketMinSize = boundedInclusive(8, 24);
    const int junkPacketMaxSize = boundedInclusive(qMax(48, junkPacketMinSize + 16), 96);

    // Keep every obfuscated packet length distinct. Reusing a single padding
    // value across all installations produces an avoidable statistical
    // signature, while S1-S4 >= 12 is required by AWG 3.1 header protection.
    QSet<int> packetSizes;
    const auto choosePadding = [&packetSizes, &boundedInclusive](int baseSize, int maximumPadding) {
        int padding = 12;
        do {
            padding = boundedInclusive(12, maximumPadding);
        } while (packetSizes.contains(baseSize + padding));
        packetSizes.insert(baseSize + padding);
        return padding;
    };
    const int initPadding = choosePadding(AwgConstant::messageInitiationSize, protocols::awg::initPacketJunkSizeMax);
    const int responsePadding = choosePadding(AwgConstant::messageResponseSize, protocols::awg::responsePacketJunkSizeMax);
    const int cookiePadding = choosePadding(AwgConstant::messageCookieReplySize, protocols::awg::cookieReplyPacketJunkSizeMax);
    const int transportPadding = choosePadding(AwgConstant::messageTransportSize, protocols::awg::cookieReplyPacketJunkSizeMax);

    QSet<quint32> headers;
    const auto chooseHeader = [&headers, random]() {
        quint32 value = 0;
        do {
            value = random->generate() & 0x7fffffffU;
        } while (value < 5 || headers.contains(value));
        headers.insert(value);
        return QString::number(value);
    };

    serverConfig.junkPacketCount = QString::number(junkPacketCount);
    serverConfig.junkPacketMinSize = QString::number(junkPacketMinSize);
    serverConfig.junkPacketMaxSize = QString::number(junkPacketMaxSize);
    serverConfig.initPacketJunkSize = QString::number(initPadding);
    serverConfig.responsePacketJunkSize = QString::number(responsePadding);
    serverConfig.cookieReplyPacketJunkSize = QString::number(cookiePadding);
    serverConfig.transportPacketJunkSize = QString::number(transportPadding);

    serverConfig.initPacketMagicHeader = chooseHeader();
    serverConfig.responsePacketMagicHeader = chooseHeader();
    serverConfig.underloadPacketMagicHeader = chooseHeader();
    serverConfig.transportPacketMagicHeader = chooseHeader();

    serverConfig.headerProtectionKey = WireguardConfigurator::genClientKeys().clientPrivKey;
    const int contentPaddingMin = boundedInclusive(10, 30);
    const int contentPaddingMax = boundedInclusive(90, 150);
    serverConfig.contentPaddingAddition = QStringLiteral("%1-%2")
            .arg(contentPaddingMin)
            .arg(contentPaddingMax);
    serverConfig.rekeyAfterTime = protocols::awg::defaultRekeyAfterTime;
    serverConfig.rekeyTimeout = protocols::awg::defaultRekeyTimeout;
    serverConfig.rejectAfterTime = protocols::awg::defaultRejectAfterTime;
    serverConfig.keepaliveTimeout = protocols::awg::defaultKeepaliveTimeout;
    serverConfig.maxHandshakeAttempts = protocols::awg::defaultMaxHandshakeAttempts;
    serverConfig.randomTrailers = protocols::awg::defaultRandomTrailers;
    serverConfig.disableCookies = protocols::awg::defaultDisableCookies;

    // Per-install random signature packets avoid giving every DP WG server the
    // same pre-handshake byte pattern. I-packets do not carry key material.
    serverConfig.specialJunk1 = QStringLiteral("<r %1>").arg(boundedInclusive(32, 96));
    serverConfig.specialJunk2 = random->bounded(2) == 0
            ? QString()
            : QStringLiteral("<r %1>").arg(boundedInclusive(24, 80));
    serverConfig.specialJunk3.clear();
    serverConfig.specialJunk4.clear();
    serverConfig.specialJunk5.clear();
}

ErrorCode AwgInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                   SshSession* sshSession, ContainerConfig &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    
    // Use appropriate config path based on container type
    QString configPath = protocols::awg::serverConfigPath;
    if (container == DockerContainer::Awg) {
        configPath = protocols::awg::serverLegacyConfigPath;
    }
    
    QString serverConfig = sshSession->getTextFileFromContainer(container, credentials, configPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QMap<QString, QString> serverConfigMap;
    auto serverConfigLines = serverConfig.split("\n");
    for (auto &line : serverConfigLines) {
        auto trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                serverConfigMap.insert(parts[0].trimmed(), parts[1].trimmed());
            }
        }
    }

    if (auto* awgConfig = config.getAwgProtocolConfig()) {
        QString addressValue = serverConfigMap.value("Address");
        QStringList addressParts = addressValue.split("/");
        awgConfig->serverConfig.subnetAddress = addressParts.value(0);
        if (addressParts.size() > 1) {
            awgConfig->serverConfig.subnetCidr = addressParts.value(1);
        }
        awgConfig->serverConfig.junkPacketCount = serverConfigMap.value(configKey::junkPacketCount);
        awgConfig->serverConfig.junkPacketMinSize = serverConfigMap.value(configKey::junkPacketMinSize);
        awgConfig->serverConfig.junkPacketMaxSize = serverConfigMap.value(configKey::junkPacketMaxSize);
        awgConfig->serverConfig.initPacketJunkSize = serverConfigMap.value(configKey::initPacketJunkSize);
        awgConfig->serverConfig.responsePacketJunkSize = serverConfigMap.value(configKey::responsePacketJunkSize);
        awgConfig->serverConfig.initPacketMagicHeader = serverConfigMap.value(configKey::initPacketMagicHeader);
        awgConfig->serverConfig.responsePacketMagicHeader = serverConfigMap.value(configKey::responsePacketMagicHeader);
        awgConfig->serverConfig.underloadPacketMagicHeader = serverConfigMap.value(configKey::underloadPacketMagicHeader);
        awgConfig->serverConfig.transportPacketMagicHeader = serverConfigMap.value(configKey::transportPacketMagicHeader);

        // hack to parse i1-i5 from commented lines in server config
        awgConfig->serverConfig.specialJunk1 = serverConfigMap.value(QString("# ") + configKey::specialJunk1);
        awgConfig->serverConfig.specialJunk2 = serverConfigMap.value(QString("# ") + configKey::specialJunk2);
        awgConfig->serverConfig.specialJunk3 = serverConfigMap.value(QString("# ") + configKey::specialJunk3);
        awgConfig->serverConfig.specialJunk4 = serverConfigMap.value(QString("# ") + configKey::specialJunk4);
        awgConfig->serverConfig.specialJunk5 = serverConfigMap.value(QString("# ") + configKey::specialJunk5);

        awgConfig->serverConfig.cookieReplyPacketJunkSize = serverConfigMap.value(configKey::cookieReplyPacketJunkSize);
        awgConfig->serverConfig.transportPacketJunkSize = serverConfigMap.value(configKey::transportPacketJunkSize);

        awgConfig->serverConfig.headerProtectionKey = serverConfigMap.value(configKey::headerProtectionKey);
        awgConfig->serverConfig.contentPaddingAddition = serverConfigMap.value(configKey::contentPaddingAddition);
        awgConfig->serverConfig.rekeyAfterTime = serverConfigMap.value(configKey::rekeyAfterTime);
        awgConfig->serverConfig.rekeyTimeout = serverConfigMap.value(configKey::rekeyTimeout);
        awgConfig->serverConfig.rejectAfterTime = serverConfigMap.value(configKey::rejectAfterTime);
        awgConfig->serverConfig.keepaliveTimeout = serverConfigMap.value(configKey::keepaliveTimeout);
        awgConfig->serverConfig.maxHandshakeAttempts = serverConfigMap.value(configKey::maxHandshakeAttempts);
        awgConfig->serverConfig.randomTrailers = serverConfigMap.value(configKey::randomTrailers);
        awgConfig->serverConfig.disableCookies = serverConfigMap.value(configKey::disableCookies);

        awgConfig->serverConfig.protocolVersion = awgConfig->serverProtocolVersion();
    }

    return ErrorCode::NoError;
}
