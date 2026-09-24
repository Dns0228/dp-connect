#include "connectionController.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "core/configurators/configuratorBase.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/payloadSender.h"
#include "core/utils/utilities.h"
#include "core/utils/serverConfigUtils.h"
#include "version.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

ConnectionController::ConnectionController(SecureServersRepository* serversRepository,
                                         SecureAppSettingsRepository* appSettingsRepository,
                                         VpnConnection* vpnConnection,
                                         QObject* parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_vpnConnection(vpnConnection),
      m_healthNetworkManager(new QNetworkAccessManager(this))
{
    connect(m_vpnConnection, &VpnConnection::connectionStateChanged,
            this, &ConnectionController::onVpnConnectionStateChanged);
    connect(this, &ConnectionController::openConnectionRequested, m_vpnConnection, &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::closeConnectionRequested, m_vpnConnection, &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::killSwitchModeChangedRequested, m_vpnConnection, &VpnConnection::onKillSwitchModeChanged, Qt::QueuedConnection);
#ifdef Q_OS_ANDROID
    connect(this, &ConnectionController::restoreConnectionRequested, m_vpnConnection, &VpnConnection::restoreConnection, Qt::QueuedConnection);
#endif
}

bool ConnectionController::isConnected() const
{
    return m_vpnConnection && m_vpnConnection->connectionState() == Vpn::ConnectionState::Connected;
}

QString ConnectionController::activeTransportName() const
{
    return m_activeTransportName;
}

void ConnectionController::setConnectionState(Vpn::ConnectionState state)
{
    emit connectionStateChanged(state);
}

ErrorCode ConnectionController::defaultContainerForServer(const QString &serverId, DockerContainer &container) const
{
    const auto kind = m_serversRepository->serverKind(serverId);
    switch (kind) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        return ErrorCode::NoError;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        return ErrorCode::NoError;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = m_serversRepository->nativeConfig(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        return ErrorCode::NoError;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        const auto cfg = m_serversRepository->apiV2Config(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        return ErrorCode::NoError;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV1:
    case serverConfigUtils::ConfigType::AmneziaFreeV2:
        return ErrorCode::LegacyApiV1NotSupportedError;
    case serverConfigUtils::ConfigType::Invalid:
    default:
        return ErrorCode::InternalError;
    }
}

ErrorCode ConnectionController::isConnectionSupported(const QString &serverId) const
{
    if (serverId.isEmpty()) {
        return ErrorCode::InternalError;
    }

    if (!isServiceReady()) {
        return ErrorCode::AmneziaServiceNotRunning;
    }

    const serverConfigUtils::ConfigType kind = m_serversRepository->serverKind(serverId);
    if (serverConfigUtils::isLegacyApiSubscription(kind)) {
        return ErrorCode::LegacyApiV1NotSupportedError;
    }

    DockerContainer container = DockerContainer::None;
    const ErrorCode errorCode = defaultContainerForServer(serverId, container);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (container == DockerContainer::None) {
        if (serverConfigUtils::isApiV2Subscription(kind)) {
            return ErrorCode::NoError;
        }
        return ErrorCode::NoInstalledContainersError;
    }

    if (ContainerUtils::isUnsupportedContainer(container)) {
        return ErrorCode::LegacyContainerNotSupportedError;
    }

    if (!isContainerSupported(container)) {
        return ErrorCode::NotSupportedOnThisPlatform;
    }

    return ErrorCode::NoError;
}

ErrorCode ConnectionController::prepareConnection(const QString &serverId,
                                                 QJsonObject& vpnConfiguration,
                                                 DockerContainer& container)
{
    return prepareConnectionForContainer(serverId, DockerContainer::None, vpnConfiguration, container);
}

ErrorCode ConnectionController::prepareConnectionForContainer(const QString &serverId,
                                                               DockerContainer requestedContainer,
                                                               QJsonObject &vpnConfiguration,
                                                               DockerContainer &container)
{
    ContainerConfig containerConfigModel;
    QPair<QString, QString> dns;
    QString hostName;
    QString description;
    int configVersion = 0;
    bool isApiConfig = false;

    const auto kind = m_serversRepository->serverKind(serverId);
    const QString primaryDns = m_appSettingsRepository->primaryDns();
    const QString secondaryDns = m_appSettingsRepository->secondaryDns();
    switch (kind) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) return ErrorCode::InternalError;
        container = requestedContainer == DockerContainer::None ? cfg->defaultContainer : requestedContainer;
        if (!cfg->containers.contains(container)) return ErrorCode::NoInstalledContainersError;
        containerConfigModel = cfg->containerConfig(container);
        dns = cfg->getDnsPair(m_appSettingsRepository->useAmneziaDns(), primaryDns, secondaryDns);
        hostName = cfg->hostName;
        description = cfg->description;
        break;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) return ErrorCode::InternalError;
        container = requestedContainer == DockerContainer::None ? cfg->defaultContainer : requestedContainer;
        if (!cfg->containers.contains(container)) return ErrorCode::NoInstalledContainersError;
        containerConfigModel = cfg->containerConfig(container);
        dns = cfg->getDnsPair(primaryDns, secondaryDns);
        hostName = cfg->hostName;
        description = cfg->description;
        break;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = m_serversRepository->nativeConfig(serverId);
        if (!cfg.has_value()) return ErrorCode::InternalError;
        container = requestedContainer == DockerContainer::None ? cfg->defaultContainer : requestedContainer;
        if (!cfg->containers.contains(container)) return ErrorCode::NoInstalledContainersError;
        containerConfigModel = cfg->containerConfig(container);
        dns = cfg->getDnsPair(primaryDns, secondaryDns);
        hostName = cfg->hostName;
        description = cfg->description;
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        const auto cfg = m_serversRepository->apiV2Config(serverId);
        if (!cfg.has_value()) return ErrorCode::InternalError;
        container = requestedContainer == DockerContainer::None ? cfg->defaultContainer : requestedContainer;
        if (!cfg->containers.contains(container)) return ErrorCode::NoInstalledContainersError;
        containerConfigModel = cfg->containerConfig(container);
        dns = cfg->getDnsPair(primaryDns, secondaryDns);
        hostName = cfg->hostName;
        description = cfg->description;
        configVersion = serverConfigUtils::ConfigSource::AmneziaGateway;
        isApiConfig = true;
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV1:
    case serverConfigUtils::ConfigType::AmneziaFreeV2:
        return ErrorCode::InternalError;
    case serverConfigUtils::ConfigType::Invalid:
    default:
        return ErrorCode::InternalError;
    }

    vpnConfiguration = createConnectionConfiguration(dns, isApiConfig, hostName, description, configVersion,
                                                     containerConfigModel, container);

    return ErrorCode::NoError;
}

ErrorCode ConnectionController::openConnection(const QString &serverId)
{
    const auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (apiV2.has_value() && !apiV2->sendPayload.isEmpty()) {
        PayloadSender::sendAll(apiV2->sendPayload);
    }

    resetStealthSession();
    m_userDisconnectRequested = false;
    m_activeServerId = serverId;

    if (m_appSettingsRepository->isDpStealthEnabled()) {
        m_stealthCandidates = stealthCandidatesForServer(serverId);
    }

    if (m_stealthCandidates.isEmpty()) {
        DockerContainer defaultContainer = DockerContainer::None;
        const ErrorCode defaultError = defaultContainerForServer(serverId, defaultContainer);
        if (defaultError != ErrorCode::NoError) {
            resetStealthSession();
            return defaultError;
        }
        m_stealthCandidates.append(defaultContainer);
    }

    m_stealthSessionActive = m_stealthCandidates.size() > 1;
    return openNextStealthCandidate();
}

void ConnectionController::closeConnection()
{
    if (m_vpnConnection) {
        m_userDisconnectRequested = true;
        resetStealthSession();
        emit closeConnectionRequested();
    }
}

QList<DockerContainer> ConnectionController::stealthCandidatesForServer(const QString &serverId) const
{
    QList<DockerContainer> installed;

    const auto appendInstalled = [this, &installed](const auto &config) {
        for (auto it = config.containers.cbegin(); it != config.containers.cend(); ++it) {
            const DockerContainer candidate = it.key();
            if (ContainerUtils::containerService(candidate) != ServiceType::Vpn
                    || !ContainerUtils::isSupportedByCurrentPlatform(candidate)
                    || ContainerUtils::isUnsupportedContainer(candidate)
                    || !it.value().protocolConfig.hasClientConfig()) {
                continue;
            }
            installed.append(candidate);
        }
    };

    switch (m_serversRepository->serverKind(serverId)) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto config = m_serversRepository->selfHostedAdminConfig(serverId);
        if (config.has_value()) appendInstalled(*config);
        break;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto config = m_serversRepository->selfHostedUserConfig(serverId);
        if (config.has_value()) appendInstalled(*config);
        break;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto config = m_serversRepository->nativeConfig(serverId);
        if (config.has_value()) appendInstalled(*config);
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        const auto config = m_serversRepository->apiV2Config(serverId);
        if (config.has_value()) appendInstalled(*config);
        break;
    }
    default:
        break;
    }

    // DP WG is the product's primary transport. REALITY remains the first
    // fallback for networks that suppress UDP, followed by broadly compatible
    // legacy transports.
    const QList<DockerContainer> priority {
        DockerContainer::Awg2,
        DockerContainer::Awg,
        DockerContainer::Xray,
        DockerContainer::OpenVpn,
        DockerContainer::WireGuard,
        DockerContainer::Ipsec
    };

    QList<DockerContainer> ordered;
    for (const DockerContainer candidate : priority) {
        if (installed.contains(candidate) && !ordered.contains(candidate)) {
            ordered.append(candidate);
        }
    }
    return ordered;
}

ErrorCode ConnectionController::openNextStealthCandidate()
{
    ErrorCode lastError = ErrorCode::NoInstalledContainersError;
    while (++m_stealthCandidateIndex < m_stealthCandidates.size()) {
        QJsonObject vpnConfiguration;
        DockerContainer container = DockerContainer::None;
        lastError = prepareConnectionForContainer(m_activeServerId,
                                                  m_stealthCandidates.at(m_stealthCandidateIndex),
                                                  vpnConfiguration,
                                                  container);
        if (lastError != ErrorCode::NoError) {
            continue;
        }

        qInfo() << "DP Stealth: connecting with candidate"
                << ContainerUtils::containerToString(container)
                << (m_stealthCandidateIndex + 1) << "of" << m_stealthCandidates.size();
        const QString transportName = ContainerUtils::containerHumanNames().value(container);
        if (m_activeTransportName != transportName) {
            m_activeTransportName = transportName;
            emit activeTransportChanged(m_activeTransportName);
        }
        m_switchingStealthCandidate = true;
        emit openConnectionRequested(m_activeServerId, container, vpnConfiguration);
        return ErrorCode::NoError;
    }

    resetStealthSession();
    return lastError;
}

void ConnectionController::onVpnConnectionStateChanged(Vpn::ConnectionState state)
{
    if (state == Vpn::ConnectionState::Connecting) {
        cancelStealthHealthCheck();
        m_switchingStealthCandidate = false;
        emit connectionStateChanged(state);
        return;
    }

    if (m_waitingForHealthFallback && state == Vpn::ConnectionState::Disconnecting) {
        return;
    }

    if (m_waitingForHealthFallback
            && (state == Vpn::ConnectionState::Disconnected || state == Vpn::ConnectionState::Error)) {
        m_waitingForHealthFallback = false;
        const ErrorCode error = openNextStealthCandidate();
        if (error == ErrorCode::NoError) {
            return;
        }
    }

    if (m_switchingStealthCandidate && state == Vpn::ConnectionState::Disconnected) {
        return;
    }

    if (state == Vpn::ConnectionState::Connected
            && m_stealthSessionActive
            && !m_userDisconnectRequested
            && (m_stealthCandidateIndex + 1 < m_stealthCandidates.size())) {
        startStealthHealthCheck();
        return;
    }

    const bool mayFallback = m_stealthSessionActive
            && !m_userDisconnectRequested
            && (state == Vpn::ConnectionState::Error || state == Vpn::ConnectionState::Disconnected)
            && (m_stealthCandidateIndex + 1 < m_stealthCandidates.size());

    if (mayFallback) {
        qWarning() << "DP Stealth: candidate failed, switching transport";
        const ErrorCode error = openNextStealthCandidate();
        if (error == ErrorCode::NoError) {
            return;
        }
    }

    emit connectionStateChanged(state);

    if (state == Vpn::ConnectionState::Error
            || state == Vpn::ConnectionState::Disconnected
            || state == Vpn::ConnectionState::Unknown) {
        resetStealthSession();
    }
}

void ConnectionController::startStealthHealthCheck()
{
    cancelStealthHealthCheck();

    // Two independent HTTPS endpoints reduce false fallbacks when one provider
    // is unavailable locally. A successful TLS/HTTP response proves that the
    // selected tunnel carries real traffic, rather than only reporting an
    // established local interface.
    const QList<QUrl> endpoints {
        QUrl(QStringLiteral("https://cp.cloudflare.com/generate_204")),
        QUrl(QStringLiteral("https://connectivitycheck.gstatic.com/generate_204"))
    };

    m_healthCheckInProgress = true;
    m_pendingHealthReplies = endpoints.size();
    const quint64 generation = ++m_healthCheckGeneration;

    for (const QUrl &endpoint : endpoints) {
        QNetworkRequest request(endpoint);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setRawHeader("Connection", "close");
        request.setTransferTimeout(6000);

        QNetworkReply *reply = m_healthNetworkManager->get(request);
        m_healthReplies.append(reply);
        connect(reply, &QNetworkReply::finished, this, [this, reply, generation]() {
            if (generation != m_healthCheckGeneration || !m_healthCheckInProgress) {
                reply->deleteLater();
                return;
            }

            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool reachable = reply->error() == QNetworkReply::NoError && status == 204;
            reply->deleteLater();

            if (reachable) {
                finishStealthHealthCheck(true);
                return;
            }

            if (--m_pendingHealthReplies == 0) {
                finishStealthHealthCheck(false);
            }
        });
    }
}

void ConnectionController::finishStealthHealthCheck(bool reachable)
{
    if (!m_healthCheckInProgress) {
        return;
    }

    m_healthCheckInProgress = false;
    ++m_healthCheckGeneration;
    m_pendingHealthReplies = 0;

    const auto replies = m_healthReplies;
    m_healthReplies.clear();
    for (const QPointer<QNetworkReply> &reply : replies) {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    }

    if (reachable) {
        qInfo() << "DP Stealth: transport health check passed";
        emit connectionStateChanged(Vpn::ConnectionState::Connected);
        return;
    }

    qWarning() << "DP Stealth: tunnel established without internet access, switching transport";
    if (m_stealthCandidateIndex + 1 < m_stealthCandidates.size()) {
        m_waitingForHealthFallback = true;
        m_switchingStealthCandidate = true;
        emit closeConnectionRequested();
        return;
    }

    emit connectionStateChanged(Vpn::ConnectionState::Error);
    resetStealthSession();
}

void ConnectionController::cancelStealthHealthCheck()
{
    ++m_healthCheckGeneration;
    m_healthCheckInProgress = false;
    m_pendingHealthReplies = 0;

    const auto replies = m_healthReplies;
    m_healthReplies.clear();
    for (const QPointer<QNetworkReply> &reply : replies) {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    }
}

void ConnectionController::resetStealthSession()
{
    cancelStealthHealthCheck();
    m_activeServerId.clear();
    m_stealthCandidates.clear();
    m_stealthCandidateIndex = -1;
    m_stealthSessionActive = false;
    m_switchingStealthCandidate = false;
    m_waitingForHealthFallback = false;
    if (!m_activeTransportName.isEmpty()) {
        m_activeTransportName.clear();
        emit activeTransportChanged(m_activeTransportName);
    }
}

#ifdef Q_OS_ANDROID
void ConnectionController::restoreConnection()
{
    if (m_vpnConnection) {
        emit restoreConnectionRequested();
    }
}
#endif

void ConnectionController::onKillSwitchModeChanged(bool enabled)
{
    if (m_vpnConnection) {
        emit killSwitchModeChangedRequested(enabled);
    }
}

ErrorCode ConnectionController::lastConnectionError() const
{
    return m_vpnConnection->lastError();
}

QJsonObject ConnectionController::createConnectionConfiguration(const QPair<QString, QString> &dns,
                                                              bool isApiConfig,
                                                              const QString &hostName,
                                                              const QString &description,
                                                              int configVersion,
                                                              const ContainerConfig &containerConfig,
                                                              DockerContainer container)
{
    QJsonObject vpnConfiguration {};

    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    Proto proto = ContainerUtils::defaultProtocol(container);

    ConnectionSettings connectionSettings = {
        { dns.first, dns.second },
        isApiConfig,
        {
            m_appSettingsRepository->isSitesSplitTunnelingEnabled(),
            m_appSettingsRepository->routeMode()
        }
    };

    auto configurator = ConfiguratorBase::create(proto, nullptr);
    ProtocolConfig processedConfig = configurator->processConfigWithLocalSettings(connectionSettings,
                                                                                  containerConfig.protocolConfig);

    QJsonObject vpnConfigData = processedConfig.getClientConfigJson();
    if (ContainerUtils::isAwgContainer(container) || container == DockerContainer::WireGuard) {
        if (vpnConfigData[configKey::mtu].toString().isEmpty()) {
            vpnConfigData[configKey::mtu] =
                    ContainerUtils::isAwgContainer(container) ? protocols::awg::defaultMtu :
                    protocols::wireguard::defaultMtu;
        }
    }

    vpnConfiguration.insert(ProtocolUtils::key_proto_config_data(proto), vpnConfigData);
    vpnConfiguration[configKey::vpnProto] = ProtocolUtils::protoToString(proto);

    vpnConfiguration[configKey::dns1] = dns.first;
    vpnConfiguration[configKey::dns2] = dns.second;

    vpnConfiguration[configKey::hostName] = hostName;
    vpnConfiguration[configKey::description] = description;
    vpnConfiguration[configKey::configVersion] = configVersion;

    return vpnConfiguration;
}

bool ConnectionController::isServiceReady() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    return Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true);
#else
    return true;
#endif
}

bool ConnectionController::isContainerSupported(DockerContainer container) const
{
    return ContainerUtils::isSupportedByCurrentPlatform(container);
}
