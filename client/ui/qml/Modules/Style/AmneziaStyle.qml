pragma Singleton

import QtQuick

QtObject {
    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'
        readonly property color paleGray: '#F3F6F5'
        readonly property color lightGray: '#D0DEDA'
        readonly property color mutedGray: '#94AAA5'
        readonly property color charcoalGray: '#4B6663'
        readonly property color slateGray: '#24353C'
        readonly property color onyxBlack: '#19242B'
        readonly property color midnightBlack: '#10171C'
        readonly property color goldenApricot: goldenApricotString
        readonly property color benefitsPanelBackground: '#19242B'
        readonly property color softViolet: '#A9C7F0'
        // Keep legacy token names so all existing controls share the DP palette.
        readonly property color burntOrange: '#319C80'
        readonly property color mutedBrown: '#4C7E73'
        readonly property color richBrown: '#1D6554'
        readonly property color deepBrown: '#173E36'
        readonly property color vibrantRed: '#F17880'
        readonly property color vibrantGreen: '#77E0C2'
        readonly property color deepMagenta: '#A64F65'
        readonly property color darkCharcoal: '#142C29'
        readonly property color pearlGray: '#F3F6F5'

        readonly property color sheerWhite: Qt.rgba(1, 1, 1, 0.12)
        readonly property color translucentWhite: Qt.rgba(1, 1, 1, 0.08)
        readonly property color barelyTranslucentWhite: Qt.rgba(1, 1, 1, 0.05)
        readonly property color translucentMidnightBlack: Qt.alpha(midnightBlack, 0.8)
        readonly property color softGoldenApricot: Qt.alpha(goldenApricot, 0.3)
        readonly property color mistyGray: Qt.alpha(paleGray, 0.8)
        readonly property color cloudyGray: Qt.alpha(paleGray, 0.65)
        readonly property color translucentRichBrown: Qt.alpha(richBrown, 0.26)
        readonly property color translucentSlateGray: Qt.alpha(slateGray, 0.13)
        readonly property color translucentOnyxBlack: Qt.alpha(onyxBlack, 0.13)

        readonly property string goldenApricotString: '#77E0C2'

        readonly property color backgroundBase: midnightBlack
        readonly property color surfaceBase: onyxBlack
        readonly property color surfaceHovered: '#203139'
        readonly property color surfacePressed: slateGray
        readonly property color surfaceInverse: paleGray
        readonly property color surfaceInverseHovered: lightGray
        readonly property color surfaceInversePressed: mutedGray
        readonly property color textPrimary: paleGray
        readonly property color textTertiary: mutedGray
        readonly property color textInverted: midnightBlack
        readonly property color textStaticWhite: paleGray
        readonly property color borderSoft: '#344C52'
        readonly property color accentSuccess: goldenApricot
        readonly property color accentWarning: '#EBC275'
    }
}
