import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"

PageType {
    id: root

    property var report: ({})
    property bool checkCompleted: false

    function yesNo(value) {
        return value === true || value === "true" ? qsTr("Ready") : qsTr("Needs attention")
    }

    function runCheck() {
        checkCompleted = false
        PageController.showBusyIndicator(true)
        report = InstallController.checkServerPreflight()
        PageController.showBusyIndicator(false)
        checkCompleted = report.completed === true
    }

    Component.onCompleted: runCheck()

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12

        header: ColumnLayout {
            width: listView.width
            spacing: 12

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerTextMaximumLineCount: 3
                headerText: qsTr("Server readiness check")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("DP Connect checked the VPS before installation. No server settings were changed.")
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: root.checkCompleted && !root.report.ready
                iconPath: "qrc:/images/controls/alert-circle.svg"
                backGroundColor: AmneziaStyle.color.translucentWhite
                textString: qsTr("Installation cannot start until architecture, package manager, and administrator access are supported.")
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: root.checkCompleted && root.report.ready
                         && (!root.report.memory_recommended || !root.report.disk_recommended)
                iconPath: "qrc:/images/controls/alert-circle.svg"
                backGroundColor: AmneziaStyle.color.translucentWhite
                textString: qsTr("The VPS is compatible, but 512 MB RAM and 2 GB of free disk space are recommended.")
            }
        }

        model: root.checkCompleted ? [
            { title: qsTr("Operating system"), value: root.report.os_id + " " + root.report.os_version, ok: true },
            { title: qsTr("Architecture"), value: root.report.architecture, ok: root.report.architecture_supported },
            { title: qsTr("Memory"), value: root.report.memory_mb + " MB", ok: root.report.memory_recommended },
            { title: qsTr("Free disk space"), value: root.report.disk_free_mb + " MB", ok: root.report.disk_recommended },
            { title: qsTr("Package manager"), value: root.report.package_manager, ok: root.report.package_manager_supported },
            { title: qsTr("Administrator access"), value: root.yesNo(root.report.sudo_ready), ok: root.report.sudo_ready },
            { title: qsTr("Docker"), value: root.report.docker_installed === "true" ? qsTr("Installed") : qsTr("Will be installed"), ok: true }
        ] : []

        delegate: Rectangle {
            required property var modelData
            width: ListView.view.width - 32
            x: 16
            implicitHeight: row.implicitHeight + 24
            radius: 12
            color: AmneziaStyle.color.translucentWhite

            RowLayout {
                id: row
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                LabelTextType {
                    Layout.fillWidth: true
                    text: modelData.title
                }

                CaptionTextType {
                    text: modelData.value
                    color: modelData.ok ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.paleGray
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width
            spacing: 12

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Continue")
                enabled: root.checkCompleted && root.report.ready
                clickedFunc: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardEasy)
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24
                text: qsTr("Check again")
                clickedFunc: root.runCheck
            }
        }
    }
}
