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

    property var overview: ({})
    property bool loading: true

    function refresh() {
        loading = true
        var container = ServersUiController.serverDefaultContainer(ServersUiController.processedServerId)
        InstallController.refreshServerOverview(ServersUiController.processedServerId, container)
    }

    function uptimeText(startedAt) {
        // Docker may return nanoseconds while JavaScript Date accepts milliseconds.
        var normalized = String(startedAt).replace(/(\.\d{3})\d+Z$/, "$1Z")
        var started = Date.parse(normalized)
        if (isNaN(started)) return qsTr("Unknown")
        var minutes = Math.max(0, Math.floor((Date.now() - started) / 60000))
        var days = Math.floor(minutes / 1440)
        var hours = Math.floor((minutes % 1440) / 60)
        if (days > 0) return qsTr("%1 d %2 h").arg(days).arg(hours)
        if (hours > 0) return qsTr("%1 h %2 min").arg(hours).arg(minutes % 60)
        return qsTr("%1 min").arg(minutes)
    }

    Component.onCompleted: refresh()

    Connections {
        target: InstallController
        function onServerOverviewRefreshed(data) {
            root.overview = data
            root.loading = false
        }
    }

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
            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Server status")
                descriptionText: root.loading ? qsTr("Checking…") : root.overview.container
            }
        }

        model: root.loading ? [] : [
            { title: qsTr("Container"), value: root.overview.status === "running" ? qsTr("Running") : root.overview.status },
            { title: qsTr("Uptime"), value: root.uptimeText(root.overview.started_at) },
            { title: qsTr("Protocol version"), value: root.overview.version || qsTr("Unknown") },
            { title: qsTr("Devices"), value: root.overview.peers || "0" },
            { title: qsTr("Image"), value: root.overview.image || qsTr("Unknown") },
            { title: qsTr("Last check"), value: new Date(root.overview.checked_at).toLocaleString() }
        ]

        delegate: Rectangle {
            required property var modelData
            width: listView.width - 32
            x: 16
            implicitHeight: content.implicitHeight + 24
            radius: 12
            color: AmneziaStyle.color.translucentWhite

            RowLayout {
                id: content
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                LabelTextType { Layout.fillWidth: true; text: modelData.title }
                CaptionTextType {
                    Layout.maximumWidth: listView.width * 0.48
                    horizontalAlignment: Text.AlignRight
                    wrapMode: Text.WrapAnywhere
                    text: modelData.value
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width - 32
            x: 16
            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.bottomMargin: 24
                text: qsTr("Refresh")
                enabled: !root.loading
                clickedFunc: root.refresh
            }
        }
    }
}
