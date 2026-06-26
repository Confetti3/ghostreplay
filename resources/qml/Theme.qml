import QtQuick

QtObject {
    readonly property color shell: "#050811"
    readonly property color chrome: "#070A13"
    readonly property color rail: "#070A13"
    readonly property color surface: "#050811"
    readonly property color surfaceAlt: "#0D1527"
    readonly property color surfaceRaised: "#111A30"
    readonly property color surfaceStrong: "#1B2A4A"
    readonly property color hover: "#17223D"
    readonly property color pressed: "#213054"
    readonly property color selected: "#17223D"
    readonly property color overlay: "#050811"
    readonly property color border: "#1B2A4A"
    readonly property color borderStrong: "#334E80"
    readonly property color innerLine: Qt.rgba(1.0, 1.0, 1.0, 0.04)
    readonly property color textPrimary: "#E2E8F0"
    readonly property color textSecondary: "#94A3B8"
    readonly property color textMuted: "#64748B"
    readonly property color textSoft: "#475569"
    readonly property color accent: "#2563EB"
    readonly property color accentStrong: "#3B82F6"
    readonly property color accentSoft: "#1D4ED8"
    readonly property color accentBlue: "#2563EB"
    readonly property color accentCyan: "#06B6D4"
    readonly property color accentGreen: "#10B981"
    readonly property color accentPurple: "#8B5CF6"
    readonly property color accentAmber: "#F59E0B"
    readonly property color accentRed: "#EF4444"
    readonly property color danger: "#EF4444"
    readonly property color dangerSoft: "#450A0A"
    readonly property color inactive: "#475569"

    readonly property int radiusSm: 0
    readonly property int radiusMd: 0
    readonly property int radiusLg: 0
    readonly property int gapXs: 6
    readonly property int gapSm: 8
    readonly property int gapMd: 12
    readonly property int gapLg: 16
    readonly property int gapXl: 24

    readonly property string displayFamily: "Segoe UI"
    readonly property string bodyFamily: "Segoe UI"
    readonly property string monoFamily: "Cascadia Code"
    readonly property font titleFont: Qt.font({ family: displayFamily, pixelSize: 14, weight: Font.Medium })
    readonly property font headingFont: Qt.font({ family: displayFamily, pixelSize: 20, weight: Font.DemiBold })
    readonly property font bodyFont: Qt.font({ family: bodyFamily, pixelSize: 13 })
    readonly property font smallFont: Qt.font({ family: bodyFamily, pixelSize: 11 })
    readonly property font monoFont: Qt.font({ family: "Cascadia Code", pixelSize: 11 })
    readonly property font iconFont: Qt.font({ family: "Segoe Fluent Icons", pixelSize: 12 })
}
