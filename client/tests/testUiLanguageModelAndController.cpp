#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

#include "utils/testCoreController.h"
#include "core/models/serverDescription.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiLanguageModelAndController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController;
    SecureQSettings *m_settings;

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        m_settings->clearSettings();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString{});
        }
    }

    void testChangeLanguage()
    {
        QVERIFY2(m_coreController->m_languageModel->rowCount() > 0, "Language model should not be empty");

        QSignalSpy updateTranslationsSpy(m_coreController->m_languageUiController, &LanguageUiController::updateTranslations);
        QSignalSpy translationsUpdatedSpy(m_coreController->m_languageUiController, &LanguageUiController::translationsUpdated);

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::China_cn);
        QVERIFY2(updateTranslationsSpy.count() == 1, "updateTranslations signal should be emitted");
        QVERIFY2(translationsUpdatedSpy.count() == 1, "translationsUpdated signal should be emitted");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QVERIFY2(updateTranslationsSpy.count() == 2, "updateTranslations signal should be emitted");
        QVERIFY2(translationsUpdatedSpy.count() == 2, "translationsUpdated signal should be emitted");
    }

    void testUrl()
    {
        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::Russian);
        QString siteRU = m_coreController->m_languageUiController->getCurrentSiteUrl("test_path");
        QString docsRU = m_coreController->m_languageUiController->getCurrentDocsUrl(
                "troubleshooting/error-codes/#error-101-internalerror");
        QString hostRU = m_coreController->m_languageUiController->getCurrentHostUrl();

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QString siteEN = m_coreController->m_languageUiController->getCurrentSiteUrl("test_path");
        QString docsEN = m_coreController->m_languageUiController->getCurrentDocsUrl(
                "troubleshooting/error-codes/#error-101-internalerror");
        QString hostEN = m_coreController->m_languageUiController->getCurrentHostUrl();

        QCOMPARE(siteRU, QStringLiteral("https://github.com/Dns0228/dp-connect"));
        QCOMPARE(siteRU, siteEN);
        QCOMPARE(docsRU, QStringLiteral("https://github.com/Dns0228/dp-connect/blob/main/docs/ERROR_CODES.md#error-101-internalerror"));
        QCOMPARE(docsRU, docsEN);
        QCOMPARE(hostRU, QStringLiteral("https://github.com/Dns0228/dp-connect/blob/main/docs/SELF_HOSTING.md"));
        QCOMPARE(hostRU, hostEN);
        QVERIFY(!siteRU.contains(QStringLiteral("amnezia"), Qt::CaseInsensitive));
        QVERIFY(!docsRU.contains(QStringLiteral("amnezia"), Qt::CaseInsensitive));
        QVERIFY(!hostRU.contains(QStringLiteral("amnezia"), Qt::CaseInsensitive));
    }

    void testLineHeight()
    {
        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::Burmese);
        QVERIFY2(m_coreController->m_languageUiController->getLineHeightAppend() == 10, "line height should be 10");

        m_coreController->m_languageUiController->changeLanguage(LanguageSettings::AvailableLanguageEnum::English);
        QVERIFY2(m_coreController->m_languageUiController->getLineHeightAppend() == 0, "line height should be 0");
    }
};

QTEST_MAIN(TestUiLanguageModelAndController)
#include "testUiLanguageModelAndController.moc"
