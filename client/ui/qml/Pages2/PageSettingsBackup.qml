import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Components"
import "../Controls2/TextTypes"

PageType {
    id: root

    property bool isRestoringBackup: false

    Connections {
        target: SettingsController

        function onRestoreBackupFinished() {
            PageController.showNotificationMessage(qsTr("Settings restored from backup file"))
            PageController.goToPageHome()
        }

        function onImportBackupFromOutside(filePath) {
            restoreBackup(filePath)
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        header: ColumnLayout {

            width: listView.width

            spacing: 16

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Back up your configuration")
                descriptionText: qsTr("You can save your settings to a backup file to restore them the next time you install the application.")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout { // TODO(CyAn84): add DelegateChooser when have migrated to 6.9

            width: listView.width

            spacing: 16

            WarningType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                textString: qsTr("The backup will contain your passwords and private keys for all servers added " +
                                 "to DP Connect. Keep this information in a secure place.")

                iconPath: "qrc:/images/controls/alert-circle.svg"
            }

            TextFieldWithHeaderType {
                id: backupPassword
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Backup password")
                textField.echoMode: TextInput.Password
                textField.maximumLength: 256
                textField.placeholderText: qsTr("At least 10 characters")
            }

            TextFieldWithHeaderType {
                id: backupPasswordConfirmation
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Repeat password")
                textField.echoMode: TextInput.Password
                textField.maximumLength: 256
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: backupPassword.textField.text.length > 0
                         && (backupPassword.textField.text.length < 10
                             || backupPassword.textField.text !== backupPasswordConfirmation.textField.text)
                textString: backupPassword.textField.text.length < 10
                            ? qsTr("Use at least 10 characters")
                            : qsTr("Passwords do not match")
                iconPath: "qrc:/images/controls/alert-circle.svg"
            }

            BasicButtonType {
                id: makeBackupButton

                Layout.fillWidth: true
                Layout.topMargin: 14
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Make a backup")
                enabled: backupPassword.textField.text.length >= 10
                         && backupPassword.textField.text === backupPasswordConfirmation.textField.text

                clickedFunc: function() {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = "DPConnect.backup"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save backup file"),
                                                                qsTr("Backup files (*.backup)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/DPConnect",
                                                                true,
                                                                ".backup")
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        var saved = SettingsController.backupAppConfig(fileName, backupPassword.textField.text)
                        PageController.showBusyIndicator(false)
                        if (saved) {
                            PageController.showNotificationMessage(qsTr("Backup file saved"))
                        }
                    }
                }
            }

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
            }

            TextFieldWithHeaderType {
                id: restorePassword
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Password for encrypted backup")
                textField.echoMode: TextInput.Password
                textField.maximumLength: 256
                textField.placeholderText: qsTr("Leave empty for an old backup")
            }

            BasicButtonType {
                id: restoreBackupButton

                Layout.fillWidth: true
                Layout.topMargin: -8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                enabled: !root.isRestoringBackup

                text: qsTr("Restore from backup")

                clickedFunc: function() {
                    if (root.isRestoringBackup) {
                        return
                    }
                    var filePath = SystemController.getFileName(qsTr("Open backup file"),
                                                                qsTr("Backup files (*.backup)"))
                    if (filePath !== "") {
                        restoreBackup(filePath)
                    }
                }
            }
        }
    }

    function restoreBackup(filePath) {
        if (root.isRestoringBackup) {
            return
        }

        var headerText = qsTr("Import settings from a backup file?")
        var descriptionText = qsTr("All current settings will be reset");
        var yesButtonText = qsTr("Continue")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            if (ConnectionController.isConnected) {
                PageController.showNotificationMessage(qsTr("Cannot restore backup settings during active connection"))
            } else {
                root.isRestoringBackup = true
                PageController.showBusyIndicator(true)
                Qt.callLater(function() {
                    SettingsController.restoreAppConfig(filePath, restorePassword.textField.text)
                    PageController.showBusyIndicator(false)
                    root.isRestoringBackup = false
                })
            }
        }
        var noButtonFunction = function() {
        }

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}
