#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <memory>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/protocols/vpnProtocol.h"
#include "vpnConnection.h"

using namespace amnezia;

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionController(SecureServersRepository* serversRepository,
                                 SecureAppSettingsRepository* appSettingsRepository,
                                 VpnConnection* vpnConnection,
                                 QObject* parent = nullptr);
    ~ConnectionController() = default;

    ErrorCode prepareConnection(const QString &serverId,
                               QJsonObject& vpnConfiguration,
                               DockerContainer& container);

    ErrorCode isConnectionSupported(const QString &serverId) const;

    ErrorCode openConnection(const QString &serverId);

    void closeConnection();

#ifdef Q_OS_ANDROID
    void restoreConnection();
#endif

    void onKillSwitchModeChanged(bool enabled);

    ErrorCode lastConnectionError() const;

    bool isConnected() const;
    QString activeTransportName() const;
    void setConnectionState(Vpn::ConnectionState state);

    QJsonObject createConnectionConfiguration(const QPair<QString, QString> &dns,
                                             bool isApiConfig,
                                             const QString &hostName,
                                             const QString &description,
                                             int configVersion,
                                             const ContainerConfig &containerConfig,
                                             DockerContainer container);

    bool isServiceReady() const;

    bool isContainerSupported(DockerContainer container) const;

signals:
    void connectionStateChanged(Vpn::ConnectionState state);
    void activeTransportChanged(const QString &transportName);
    void openConnectionRequested(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration);
    void closeConnectionRequested();
    void killSwitchModeChangedRequested(bool enabled);

#ifdef Q_OS_ANDROID
    void restoreConnectionRequested();
#endif

private:
    ErrorCode defaultContainerForServer(const QString &serverId, DockerContainer &container) const;
    ErrorCode prepareConnectionForContainer(const QString &serverId,
                                            DockerContainer requestedContainer,
                                            QJsonObject &vpnConfiguration,
                                            DockerContainer &container);
    QList<DockerContainer> stealthCandidatesForServer(const QString &serverId) const;
    ErrorCode openNextStealthCandidate();
    void onVpnConnectionStateChanged(Vpn::ConnectionState state);
    void resetStealthSession();

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
    VpnConnection* m_vpnConnection;

    QString m_activeServerId;
    QList<DockerContainer> m_stealthCandidates;
    qsizetype m_stealthCandidateIndex = -1;
    bool m_stealthSessionActive = false;
    bool m_switchingStealthCandidate = false;
    bool m_userDisconnectRequested = false;
    QString m_activeTransportName;
};

#endif
