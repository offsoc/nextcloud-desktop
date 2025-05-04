/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include <qglobal.h>
#include <QTemporaryDir>
#include <QtTest>

#include "QtTest/qtestcase.h"
#include "common/utility.h"
#include "accountstate.h"
#include <accountmanager.h>
#include "configfile.h"
#include "syncenginetestutils.h"
#include "testhelper.h"

using namespace OCC;

static QByteArray fake400Response = R"(
{"ocs":{"meta":{"status":"failure","statuscode":400,"message":"Parameter is incorrect.\n"},"data":[]}}
)";

bool itemDidCompleteSuccessfully(const ItemCompletedSpy &spy, const QString &path)
{
    if (auto item = spy.findItem(path)) {
        return item->_status == SyncFileItem::Success;
    }
    return false;
}

class TestMigration: public QObject
{
    Q_OBJECT

    ConfigFile configFile;
    QTemporaryDir temporaryDir;

private:
    static constexpr char legacyAppName[] = "ownCloud";
    static constexpr char standardAppName[] = "Nextcloud";
    static constexpr char brandedAppName[] = "Branded";
    static constexpr char legacyAppConfigContent[] = "[General]\n"
        "clientVersion=5.3.2.15463\n"
        "issuesWidgetFilter=FatalError, BlacklistedError, Excluded, Message, FilenameReserved\n"
        "logHttp=false\n"
        "optionalDesktopNotifications=true\n"
        "\n"
        "[Accounts]e\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\davUrl=@Variant(http://oc.de/remote.php/dav/files/admin/)\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\deployed=false\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\displayString=ownCloud\n"
        "0\\Folders\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\ignoreHiddenFiles=true\n"
        "0\\Folders\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\journalPath=.sync_journal.db\n"
        "0\\Folders\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\localPath=/ownCloud/\n"
        "0\\Folders\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\paused=false\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\priority=0\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\targetPath=/\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\version=13\n"
        "0\\Folders\\2ba4b09a-1223-aaaa-abcd-c2df238816d8\\virtualFilesMode=off\n"
        "0\\capabilities=@QVariant()\n"
        "0\\dav_user=admin\n"
        "0\\default_sync_root=/ownCloud\n"
        "0\\display-name=admin\n"
        "0\\http_CredentialVersion=1\n"
        "0\\http_oauth=false\n"
        "0\\http_user=admin\n"
        "0\\supportsSpaces=true\n"
        "0\\url=http://oc.de/\n"
        "0\\user=admin\n"
        "0\\userExplicitlySignedOut=false\n"
        "0\\uuid=@Variant()\n"
        "0\\version=13\n"
        "version=13\n"
        "\n"
        "[Credentials]\n"
        "ownCloud_credentials%oc.de%2ba4b09a-1223-aaaa-abcd-c2df238816d8\\http\\password=true";

private slots:
    void setupConfigFileAccount() {
        QScopedPointer<FakeQNAM> fakeQnam(new FakeQNAM({}));
        OCC::AccountPtr account = OCC::Account::create();
        account->setDavUser("user");
        account->setDavDisplayName("Nextcloud user");
        account->setProxyType(QNetworkProxy::ProxyType::HttpProxy);
        account->setProxyUser("proxyuser");
        account->setDownloadLimit(120);
        account->setUploadLimit(120);
        account->setDownloadLimitSetting(OCC::Account::AccountNetworkTransferLimitSetting::ManualLimit);
        account->setServerVersion("30");
        account->setCredentials(new FakeCredentials{fakeQnam.data()});
        account->setUrl(QUrl(("http://example.de")));
        const auto accountState = OCC::AccountManager::instance()->addAccount(account);
        OCC::AccountManager::instance()->saveAccount(accountState->account());
    }

    void setupConfigFileSettings() {
        QSettings settings(configFile.configFile(), QSettings::IniFormat);
        configFile.setClientVersionString("3.0.0");
        configFile.setOptionalServerNotifications(true);
        configFile.setShowChatNotifications(true);
        configFile.setShowCallNotifications(true);
        configFile.setShowInExplorerNavigationPane(true);
        configFile.setShowInExplorerNavigationPane(true);
        configFile.setRemotePollInterval(std::chrono::milliseconds(1000));
        configFile.setAutoUpdateCheck(true, QString());
        configFile.setUpdateChannel("beta");
        configFile.setOverrideServerUrl("http://example.de");
        configFile.setOverrideLocalDir("A");
        configFile.setVfsEnabled(true);
        configFile.setProxyType(0);
        configFile.setVfsEnabled(true);
        configFile.setUseUploadLimit(0);
        configFile.setUploadLimit(1);
        configFile.setUseDownloadLimit(0);
        configFile.setUseDownloadLimit(1);
        configFile.setNewBigFolderSizeLimit(true, 500);
        configFile.setNotifyExistingFoldersOverLimit(true);
        configFile.setStopSyncingExistingFoldersOverLimit(true);
        configFile.setConfirmExternalStorage(true);
        configFile.setMoveToTrash(true);
        configFile.setForceLoginV2(true);
        configFile.setPromptDeleteFiles(true);
        configFile.setDeleteFilesThreshold(1);
        configFile.setMonoIcons(true);
        configFile.setCrashReporter(true);
        configFile.setAutomaticLogDir(true);
        configFile.setLogDir(temporaryDir.path());
        configFile.setLogDebug(true);
        configFile.setLogExpire(72);
        configFile.setLogFlush(true);
        configFile.setCertificatePath(temporaryDir.path());
        configFile.setCertificatePasswd("123456");
        configFile.setLaunchOnSystemStartup(true);
        configFile.setServerHasValidSubscription(true);
        configFile.setDesktopEnterpriseChannel("stable");
        configFile.setLanguage("pt");
        settings.sync();
    }

    // void createLegacyConfig()
    // {
    //     QTemporaryDir temporaryDir;
    //     QVERIFY(QDir(temporaryDir.path()).mkpath(standardAppName));
    //     const auto standardConfigFolder = QString(temporaryDir.path() + "/" + standardAppName);
    //     ConfigFile::setConfDir(standardConfigFolder);

    //     ConfigFile standardConfigFile;
    //     QSettings settings(standardConfigFile.configFile(), QSettings::IniFormat);
    //     standardConfigFile.setClientVersionString("3.0.0");
    //     QScopedPointer<FakeQNAM> fakeQnam(new FakeQNAM({}));

    //     OCC::AccountPtr account = OCC::Account::create();
    //     account->setDavUser("user");
    //     account->setDavDisplayName("Nextcloud user");
    //     account->setProxyType(QNetworkProxy::ProxyType::HttpProxy);
    //     account->setProxyUser("proxyuser");
    //     account->setDownloadLimit(120);
    //     account->setUploadLimit(120);
    //     account->setDownloadLimitSetting(OCC::Account::AccountNetworkTransferLimitSetting::ManualLimit);
    //     account->setServerVersion("30");
    //     account->setCredentials(new FakeCredentials{fakeQnam.data()});
    //     account->setUrl(QUrl(("http://example.de")));
    //     const auto accountState = OCC::AccountManager::instance()->addAccount(account);
    //     OCC::AccountManager::instance()->saveAccount(accountState->account());

    //     standardConfigFile.setOptionalServerNotifications(true);
    //     standardConfigFile.setShowChatNotifications(true);
    //     standardConfigFile.setShowCallNotifications(true);
    //     standardConfigFile.setShowInExplorerNavigationPane(true);
    //     standardConfigFile.setShowInExplorerNavigationPane(true);
    //     standardConfigFile.setRemotePollInterval(std::chrono::milliseconds(1000));
    //     standardConfigFile.setAutoUpdateCheck(true, QString());
    //     standardConfigFile.setUpdateChannel("beta");
    //     standardConfigFile.setOverrideServerUrl("http://example.de");
    //     standardConfigFile.setOverrideLocalDir("A");
    //     standardConfigFile.setVfsEnabled(true);
    //     standardConfigFile.setProxyType(0);
    //     standardConfigFile.setVfsEnabled(true);
    //     standardConfigFile.setUseUploadLimit(0);
    //     standardConfigFile.setUploadLimit(1);
    //     standardConfigFile.setUseDownloadLimit(0);
    //     standardConfigFile.setUseDownloadLimit(1);
    //     standardConfigFile.setNewBigFolderSizeLimit(true, 500);
    //     standardConfigFile.setNotifyExistingFoldersOverLimit(true);
    //     standardConfigFile.setStopSyncingExistingFoldersOverLimit(true);
    //     standardConfigFile.setConfirmExternalStorage(true);
    //     standardConfigFile.setMoveToTrash(true);
    //     standardConfigFile.setForceLoginV2(true);
    //     standardConfigFile.setPromptDeleteFiles(true);
    //     standardConfigFile.setDeleteFilesThreshold(1);
    //     standardConfigFile.setMonoIcons(true);
    //     standardConfigFile.setCrashReporter(true);
    //     standardConfigFile.setAutomaticLogDir(true);
    //     standardConfigFile.setLogDir(temporaryDir.path());
    //     standardConfigFile.setLogDebug(true);
    //     standardConfigFile.setLogExpire(72);
    //     standardConfigFile.setLogFlush(true);
    //     standardConfigFile.setCertificatePath(temporaryDir.path());
    //     standardConfigFile.setCertificatePasswd("123456");
    //     standardConfigFile.setLaunchOnSystemStartup(true);
    //     standardConfigFile.setServerHasValidSubscription(true);
    //     standardConfigFile.setDesktopEnterpriseChannel("stable");
    //     standardConfigFile.setLanguage("pt");
    //     settings.sync();

    //     const auto standardConfig = QString(standardConfigFolder + "/" + QString(standardAppName).toLower() + ".cfg");;
    //     QVERIFY(QFile::rename(standardConfigFile.configFile(), standardConfig));

    //     QVERIFY(QDir(temporaryDir.path()).mkpath(legacyAppName));
    //     const auto legacyConfigFolder = QString(temporaryDir.path() + "/" + legacyAppName);
    //     const auto legacyConfigFilePath = QString(legacyConfigFolder + "/" + QString(legacyAppName).toLower() + ".cfg");
    //     QFile legacyConfigFile(legacyConfigFilePath);
    //     QVERIFY(legacyConfigFile.open(QFile::WriteOnly));
    //     QCOMPARE_GE(legacyConfigFile.write(legacyAppConfigContent, qstrlen(legacyAppConfigContent)), 0);
    //     legacyConfigFile.close();
    // }

    void initTestCase()
    {
        OCC::Logger::instance()->setLogFlush(true);
        OCC::Logger::instance()->setLogDebug(true);

        QStandardPaths::setTestModeEnabled(true);
    }


    // From an existing oC client to Nextcloud
    // From the standard Nextcloud client to a branded version
    // Upgrade
    // Downgrade
    // Migration with --confdir
    void testUpgradeWithConfDir()
    {
        // create Nextcloud config with older version
        QVERIFY(QDir(temporaryDir.path()).mkpath(standardAppName));
        const auto standardConfigFolder = QString(temporaryDir.path() + "/" + standardAppName);
        ConfigFile::setConfDir(standardConfigFolder);
        setupConfigFileSettings();
        setupConfigFileAccount();


        // QTemporaryDir temporaryDir;
        // int argc = 5;
        // QByteArray appName = QString(QCoreApplication::applicationDirPath() + "/NextcloudDev").toLatin1();
        // char *arg0 = appName.data();
        // char arg1[] = "-platform";
        // char arg2[] = "offscreen";
        // char arg3[] = "--confdir";
        // QByteArray temporaryDirPath = QString(temporaryDir.path()).toLatin1();
        // char *arg4 = temporaryDirPath.data();
        // char *argv[] = { arg0, arg1, arg2, arg3, arg4, nullptr };
        // OCC::Application app(argc, argv);
        // app.setApplicationName("Nextcloud");

        // QTemporaryDir dir;
        // ConfigFile::setConfDir(dir.path()); // we don't want to pollute the user's config file

        // constexpr auto firstSharePath = "A/sharedwithme_A.txt";
        // constexpr auto secondSharePath = "A/B/sharedwithme_B.data";

        // QScopedPointer<FakeQNAM> fakeQnam(new FakeQNAM({}));
        // OCC::AccountPtr account = OCC::Account::create();
        // account->setCredentials(new FakeCredentials{fakeQnam.data()});
        // account->setUrl(QUrl(("http://example.de")));
        // OCC::AccountManager::instance()->addAccount(account);

        // FakeFolder fakeFolder{FileInfo{}};
        // fakeFolder.remoteModifier().mkdir("A");

        // fakeFolder.remoteModifier().insert(firstSharePath, 100);
        // const auto firstShare = fakeFolder.remoteModifier().find(firstSharePath);
        // QVERIFY(firstShare);
        // firstShare->permissions.setPermission(OCC::RemotePermissions::IsShared);

        // fakeFolder.remoteModifier().mkdir("A/B");

        // fakeFolder.remoteModifier().insert(secondSharePath, 100);
        // const auto secondShare = fakeFolder.remoteModifier().find(secondSharePath);
        // QVERIFY(secondShare);
        // secondShare->permissions.setPermission(OCC::RemotePermissions::IsShared);

        // FolderMan *folderman = FolderMan::instance();
        // QCOMPARE(folderman, _fm.get());
        //OCC::AccountState *accountState = OCC::AccountManager::instance()->accounts().first().data();


        // auto realFolder = FolderMan::instance()->folderForPath(fakeFolder.localPath());
        // QVERIFY(realFolder);

        // QVERIFY(fakeFolder.syncOnce());
        // QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        // fakeQnam->setOverride([this, accountState, &fakeFolder](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) {
        //     Q_UNUSED(device);
        //     QNetworkReply *reply = nullptr;

        //     if (op != QNetworkAccessManager::DeleteOperation) {
        //         reply = new FakeErrorReply(op, req, this, 405);
        //         return reply;
        //     }

        //     if (req.url().path().isEmpty()) {
        //         reply = new FakeErrorReply(op, req, this, 404);
        //         return reply;
        //     }

        //     const auto filePathRelative = req.url().path().remove(accountState->account()->davPath());

        //     const auto foundFileInRemoteFolder = fakeFolder.remoteModifier().find(filePathRelative);

        //     if (filePathRelative.isEmpty() || !foundFileInRemoteFolder) {
        //         reply = new FakeErrorReply(op, req, this, 404);
        //         return reply;
        //     }

        //     fakeFolder.remoteModifier().remove(filePathRelative);
        //     reply = new FakePayloadReply(op, req, {}, nullptr);

        //     emit incomingShareDeleted();

        //     return reply;
        // });

        // QSignalSpy incomingShareDeletedSignal(this, &TestFolderMan::incomingShareDeleted);

        //        // verify first share gets deleted
        // folderman->leaveShare(fakeFolder.localPath() + firstSharePath);
        // QCOMPARE(incomingShareDeletedSignal.count(), 1);
        // QVERIFY(!fakeFolder.remoteModifier().find(firstSharePath));
        // QVERIFY(fakeFolder.syncOnce());
        // QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        //        // verify no share gets deleted
        // folderman->leaveShare(fakeFolder.localPath() + "A/B/notsharedwithme_B.data");
        // QCOMPARE(incomingShareDeletedSignal.count(), 1);
        // QVERIFY(fakeFolder.remoteModifier().find("A/B/sharedwithme_B.data"));
        // QVERIFY(fakeFolder.syncOnce());
        // QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        //        // verify second share gets deleted
        // folderman->leaveShare(fakeFolder.localPath() + secondSharePath);
        // QCOMPARE(incomingShareDeletedSignal.count(), 2);
        // QVERIFY(!fakeFolder.remoteModifier().find(secondSharePath));
        // QVERIFY(fakeFolder.syncOnce());
        // QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        // OCC::AccountManager::instance()->deleteAccount(accountState);
    }

};

QTEST_GUILESS_MAIN(TestMigration)
#include "testmigration.moc"
