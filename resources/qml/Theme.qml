import QtQuick

QtObject {
    readonly property color shell: "#050706"
    readonly property color chrome: "#080B0D"
    readonly property color rail: "#090F12"
    readonly property color surface: "#0C1216"
    readonly property color surfaceAlt: "#11191D"
    readonly property color surfaceRaised: "#141D22"
    readonly property color surfaceStrong: "#1A252B"
    readonly property color hover: "#1A272D"
    readonly property color pressed: "#223238"
    readonly property color selected: "#182D2B"
    readonly property color overlay: "#07100F"
    readonly property color border: "#243137"
    readonly property color borderStrong: "#496260"
    readonly property color innerLine: Qt.rgba(1.0, 1.0, 1.0, 0.048)
    readonly property color textPrimary: "#F5F8FD"
    readonly property color textSecondary: "#D3DDEA"
    readonly property color textMuted: "#9FAEC0"
    readonly property color textSoft: "#74849A"
    readonly property color accent: "#8EF7C7"
    readonly property color accentStrong: "#B8FFE0"
    readonly property color accentSoft: "#12352B"
    readonly property color accentBlue: "#6EA8FF"
    readonly property color accentCyan: "#8EF7C7"
    readonly property color accentGreen: "#82E6A6"
    readonly property color accentPurple: "#C77DFF"
    readonly property color accentAmber: "#F5B84B"
    readonly property color accentRed: "#F05D5E"
    readonly property color danger: "#F05D5E"
    readonly property color dangerSoft: "#3B171A"
    readonly property color inactive: "#5D6A7B"

    readonly property int radiusSm: 8
    readonly property int radiusMd: 10
    readonly property int radiusLg: 14
    readonly property int gapXs: 6
    readonly property int gapSm: 8
    readonly property int gapMd: 12
    readonly property int gapLg: 16
    readonly property int gapXl: 24

    readonly property string displayFamily: "Space Grotesk"
    readonly property string bodyFamily: "Inter"
    readonly property string monoFamily: "Cascadia Code"
    readonly property font titleFont: Qt.font({ family: displayFamily, pixelSize: 15, weight: Font.DemiBold })
    readonly property font headingFont: Qt.font({ family: displayFamily, pixelSize: 22, weight: Font.DemiBold })
    readonly property font bodyFont: Qt.font({ family: bodyFamily, pixelSize: 13 })
    readonly property font smallFont: Qt.font({ family: bodyFamily, pixelSize: 11 })
    readonly property font monoFont: Qt.font({ family: "Cascadia Code", pixelSize: 11 })
    readonly property font iconFont: Qt.font({ family: "Segoe Fluent Icons", pixelSize: 12 })
}
