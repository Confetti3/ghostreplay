import QtQuick
import QtQuick.Effects

Item {
    id: icon

    property alias source: sourceImage.source
    property color tint: "#E2E8F0"
    property real iconOpacity: enabled ? 0.90 : 0.34
    property color glowColor: "transparent"
    property real glowStrength: 0.0
    property string treatment: "plain"
    property real lift: treatment === "featured" ? 1.0 : (treatment === "compact" ? 0.15 : 0.45)

    readonly property bool hasGlow: glowStrength > 0.0 && glowColor.a > 0.0
    readonly property bool featured: treatment === "featured" || treatment === "hero"

    implicitWidth: 16
    implicitHeight: 16

    Image {
        id: sourceImage
        anchors.fill: parent
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        visible: false
    }

    MultiEffect {
        anchors.fill: parent
        source: sourceImage
        colorization: 1.0
        colorizationColor: icon.tint
        brightness: 0.0
        saturation: 0.0
        opacity: icon.iconOpacity
    }
}
