import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType2 {
    id: root

    expandedStateContent: Item {
        id: contentRoot

        implicitHeight: content.implicitHeight + 40 + PageController.safeAreaBottomMargin

        Binding {
            target: root
            property: "expandedHeight"
            value: contentRoot.implicitHeight
        }

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true

                text: qsTr("Support")
            }

            AppTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8

                color: AmneziaStyle.color.textTertiary
                text: qsTr("For project information, visit DP project on GitHub")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("GitHub")
                descriptionText: qsTr("DP project")
                leftImageSource: "qrc:/images/controls/github.svg"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    Qt.openUrlExternally("https://github.com/Dns0228")
                }
            }

        }
    }
}
