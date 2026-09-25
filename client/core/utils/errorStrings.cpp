#include "errorStrings.h"

using namespace amnezia;

QString errorMessage(ErrorCode code) {
    QString message;

    switch (code) {

    // General error codes
    case(ErrorCode::NoError): message = QObject::tr("No error"); break;
    case(ErrorCode::UnknownError): message = QObject::tr("Unknown error"); break;
    case(ErrorCode::NotImplementedError): message = QObject::tr("Function not implemented"); break;
    case(ErrorCode::AmneziaServiceNotRunning): message = QObject::tr("Background service is not running"); break;
    case(ErrorCode::NotSupportedOnThisPlatform): message = QObject::tr("The selected protocol is not supported on the current platform"); break;

    // Server errors
    case(ErrorCode::ServerCheckFailed): message = QObject::tr("Could not check the server. Verify the VPS address, SSH access, and administrator permissions, then try again."); break;
    case(ErrorCode::ServerPortAlreadyAllocatedError): message = QObject::tr("The selected server port is already in use. Choose another port in protocol settings or stop the conflicting service on the VPS."); break;
    case(ErrorCode::ServerContainerMissingError): message = QObject::tr("Server error: Docker container missing"); break;
    case(ErrorCode::ServerDockerFailedError): message = QObject::tr("Server error: Docker failed"); break;
    case(ErrorCode::ServerCancelInstallation): message = QObject::tr("Installation canceled by user"); break;
    case(ErrorCode::ServerUserNotInSudo): message = QObject::tr("The user is not a member of the sudo group"); break;
    case(ErrorCode::ServerPacketManagerError): message = QObject::tr("Server error: Package manager error"); break;
    case(ErrorCode::ServerSudoPackageIsNotPreinstalled): message = QObject::tr("The sudo package is not pre-installed on the server"); break;
    case(ErrorCode::ServerUserDirectoryNotAccessible): message = QObject::tr("The server user's home directory is not accessible"); break;
    case(ErrorCode::ServerUserNotAllowedInSudoers): message = QObject::tr("Action not allowed in sudoers"); break;
    case(ErrorCode::ServerUserPasswordRequired): message = QObject::tr("The user's password is required"); break;
    case(ErrorCode::ServerDockerOnCgroupsV2): message = QObject::tr("Docker error: runc doesn't work on cgroups v2"); break;
    case(ErrorCode::ServerCgroupMountpoint): message = QObject::tr("Server error: cgroup mountpoint does not exist"); break;
    case(ErrorCode::DockerPullRateLimit): message = QObject::tr("Docker error: The pull rate limit has been reached"); break;
    case(ErrorCode::ServerLinuxKernelTooOld): message = QObject::tr("Server error: Linux kernel is too old"); break;
    case(ErrorCode::XrayServerConfigInvalid):
        message = QObject::tr("Server error: invalid or unreadable XRay server configuration");
        break;
    case(ErrorCode::XrayServerNoVlessClients):
        message = QObject::tr("Server error: XRay server has no VLESS clients");
        break;
    case(ErrorCode::XrayRealityKeysReadFailed):
        message = QObject::tr("Server error: failed to read XRay Reality keys from the server");
        break;
    case(ErrorCode::ServerContainerRuntimeNotSupported): message = QObject::tr("Server error: The default container runtime available for installation on this server is not supported.\n Install Docker Engine on the server manually and try again."); break;
    case(ErrorCode::ContainerRuntimeServiceNotRunning): message = QObject::tr("Container runtime error: The container runtime service is not running.\n Check the container runtime service on the server, or wait about a minute and try again."); break;

    // Libssh errors
    case(ErrorCode::SshRequestDeniedError): message = QObject::tr("SSH request was denied"); break;
    case(ErrorCode::SshInterruptedError): message = QObject::tr("SSH request was interrupted"); break;
    case(ErrorCode::SshInternalError): message = QObject::tr("SSH internal error"); break;
    case(ErrorCode::SshPrivateKeyError): message = QObject::tr("Invalid private key or invalid passphrase entered"); break;
    case(ErrorCode::SshPrivateKeyFormatError): message = QObject::tr("The selected private key format is not supported, use openssh ED25519 key types or PEM key types"); break;
    case(ErrorCode::SshTimeoutError): message = QObject::tr("Timeout connecting to server"); break;

    // Ssh scp errors
    case(ErrorCode::SshScpFailureError): message = QObject::tr("SCP error: Generic failure"); break;

    // Local errors
    case (ErrorCode::OpenVpnConfigMissing): message = QObject::tr("OpenVPN config missing"); break;
    case (ErrorCode::OpenVpnManagementServerError): message = QObject::tr("OpenVPN management server error"); break;

    // Distro errors
    case (ErrorCode::OpenVpnExecutableMissing): message = QObject::tr("OpenVPN executable missing"); break;
    case (ErrorCode::AmneziaServiceConnectionFailed): message = QObject::tr("DP Connect helper service error"); break;
    case (ErrorCode::OpenSslFailed): message = QObject::tr("OpenSSL failed"); break;

    // VPN errors
    case (ErrorCode::OpenVpnAdaptersInUseError): message = QObject::tr("Can't connect: another VPN connection is active"); break;
    case (ErrorCode::OpenVpnTapAdapterError): message = QObject::tr("Can't setup OpenVPN TAP network adapter"); break;
    case (ErrorCode::AddressPoolError): message = QObject::tr("VPN pool error: no available addresses"); break;

    case (ErrorCode::ImportInvalidConfigError): message = QObject::tr("The config does not contain any containers and credentials for connecting to the server"); break;
    case (ErrorCode::ImportBackupFileUseRestoreInstead): message = QObject::tr("Backup files cannot be imported here. Use 'Restore from backup' instead."); break;
    case (ErrorCode::RestoreBackupInvalidError): message = QObject::tr("Backup file is corrupted or has invalid format"); break;
    case (ErrorCode::LegacyApiV1NotSupportedError): message = QObject::tr("This legacy DP Connect subscription format is no longer supported"); break;
    case (ErrorCode::ConfigFormatVersionNotSupportedError): message = QObject::tr("This configuration was created in a newer version of the application and is not fully supported. Please update the application"); break;
    case (ErrorCode::RestoreBackupUnsupportedConfigsSkipped): message = QObject::tr("Some configurations from the backup were not restored because they require a newer version of the application"); break;
    case (ErrorCode::LegacyContainerNotSupportedError): message = QObject::tr("This protocol is no longer supported. Please select another protocol or remove this container from the server settings."); break;
    case (ErrorCode::ImportOpenConfigError): message = QObject::tr("Unable to open config file"); break;
    case (ErrorCode::NoInstalledContainersError): message = QObject::tr("VPN Protocols is not installed.\n Please install VPN container at first"); break;

    // Android errors
    case (ErrorCode::AndroidError): message = QObject::tr("VPN connection error"); break;

    // Api errors
    case (ErrorCode::ApiConfigDownloadError): message = QObject::tr("Error when retrieving configuration from API"); break;
    case (ErrorCode::ApiConfigAlreadyAdded): message = QObject::tr("This config has already been added to the application"); break;
    case (ErrorCode::ApiConfigEmptyError): message = QObject::tr("In the response from the server, an empty config was received"); break;
    case (ErrorCode::ApiConfigSslError): message = QObject::tr("SSL error occurred"); break;
    case (ErrorCode::ApiConfigTimeoutError): message = QObject::tr("Server response timeout on api request"); break;
    case (ErrorCode::ApiMissingAgwPublicKey): message = QObject::tr("Missing AGW public key"); break;
    case (ErrorCode::ApiConfigDecryptionError): message = QObject::tr("Failed to decrypt response payload"); break;
    case (ErrorCode::ApiServicesMissingError): message = QObject::tr("Missing list of available services"); break;
    case (ErrorCode::ApiConfigLimitError): message = QObject::tr("The limit of allowed configurations per subscription has been exceeded"); break;
    case (ErrorCode::ApiNotFoundError): message = QObject::tr("Error when retrieving configuration from API"); break;
    case (ErrorCode::ApiUpdateRequestError): message = QObject::tr("Please update the application to use this feature"); break;
    case (ErrorCode::ApiSubscriptionExpiredError): message = QObject::tr("Your VPN subscription has expired.\n Please check your email for renewal instructions.\n If you haven't received an email, please contact our support."); break;
    case (ErrorCode::ApiPurchaseError): message = QObject::tr("Unable to process purchase"); break;
    case (ErrorCode::ApiSubscriptionNotActiveError): message = QObject::tr("No active subscription found"); break;
    case (ErrorCode::ApiNoPurchasedSubscriptionsError): message = QObject::tr("No purchased subscriptions found. Please purchase a subscription first"); break;
    case (ErrorCode::ApiTrialAlreadyUsedError): message = QObject::tr("This email address has already been used to activate a trial"); break;
    case (ErrorCode::ApiCaptchaRequiredError): message = QObject::tr("CAPTCHA verification is required"); break;
    case (ErrorCode::ApiCaptchaInvalidError): message = QObject::tr("CAPTCHA was incorrect. Please try again"); break;
    case (ErrorCode::ApiCaptchaRefreshError): message = QObject::tr("CAPTCHA refreshed. Please try again"); break;
    case (ErrorCode::ApiRateLimitError): message = QObject::tr("Too many requests. Please try again later"); break;
    case (ErrorCode::ApiPurchasePendingError):
#if defined(Q_OS_ANDROID)
        message = QObject::tr("Your payment is pending confirmation in Google Play. Once the payment is completed, the subscription will be added automatically on the next app launch.");
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
        message = QObject::tr("Your payment is awaiting confirmation. Once it is approved, the subscription will be added automatically.");
#else
        message = QObject::tr("Your payment is pending confirmation. Please complete the payment and then restore your subscription.");
#endif
        break;
    case (ErrorCode::ApiNoPurchasesToRestore):
#if defined(Q_OS_ANDROID)
        message = QObject::tr("No purchases to restore. If you have an active subscription, make sure you're signed in with the same Google account used for the purchase.");
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
        message = QObject::tr("No purchases to restore. If you have an active subscription, make sure you're signed in with the same Apple ID used for the purchase.");
#else
        message = QObject::tr("No purchases to restore. If you have an active subscription, make sure you're signed in with the same account used for the purchase.");
#endif
        break;

    // QFile errors
    case(ErrorCode::OpenError): message = QObject::tr("QFile error: The file could not be opened"); break;
    case(ErrorCode::ReadError): message = QObject::tr("QFile error: An error occurred when reading from the file"); break;
    case(ErrorCode::PermissionsError): message = QObject::tr("QFile error: The file could not be accessed"); break;
    case(ErrorCode::UnspecifiedError): message =  QObject::tr("QFile error: An unspecified error occurred"); break;
    case(ErrorCode::FatalError): message =  QObject::tr("QFile error: A fatal error occurred"); break;
    case(ErrorCode::AbortError): message =  QObject::tr("QFile error: The operation was aborted"); break;

    // Billing errors
    case(ErrorCode::BillingCanceled): message = QObject::tr("Transaction was canceled by the user"); break;
    case(ErrorCode::BillingError): message = QObject::tr("Billing error"); break;
    case(ErrorCode::BillingGooglePlayError): message = QObject::tr("Internal Google Play error, please try again later"); break;
    case(ErrorCode::BillingUnavailable): message = QObject::tr("Billing is unavailable, please try again later"); break;
    case(ErrorCode::SubscriptionAlreadyOwned): message = QObject::tr("You already own this subscription"); break;
    case(ErrorCode::SubscriptionUnavailable): message = QObject::tr("The requested subscription is not available for purchase"); break;
    case(ErrorCode::BillingNetworkError): message = QObject::tr("A network error occurred during the operation, please check the Internet connection"); break;

    case(ErrorCode::InternalError):
    default:
        message = QObject::tr("DP Connect could not complete the operation. Restart the application and try again."); break;
    }

    return message;
}

QString errorString(ErrorCode code)
{
    return QObject::tr("ErrorCode: %1. ").arg(code) + errorMessage(code);
}

QDebug operator<<(QDebug debug, const ErrorCode &e)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ErrorCode::" << int(e) << "(" << errorString(e) << ")";

    return debug;
}
