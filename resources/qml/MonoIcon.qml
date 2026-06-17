import QtQuick
import QtQuick.Effects

Item {
    id: icon

    property alias source: sourceImage.source
    property color tint: "#EAF3FF"
    property real iconOpacity: enabled ? 0.90 : 0.34
    property color glowColor: "transparent"
    property real glowStrength: 0.0
    property string treatment: "plain"
    property real lift: treatment === "featured" ? 1.0 : (treatment === "compact" ? 0.15 : 0.45)

    readonly property bool hasGlow: glowStrength > 0.0 && glowColor.a > 0.0
    readonly property bool featured: treatment === "featured" || treatment === "hero"

    implicitWidth: 16
    implicitHeight: 16

    Rectangle {
        visible: icon.hasGlow
        anchors.centerIn: parent
        width: Math.max(parent.width, parent.height) * (icon.featured ? 1.85 : 1.45)
        height: width
        radius: width / 2
        color: icon.glowColor
        opacity: 0.045 + icon.glowStrength * (icon.featured ? 0.14 : 0.10)
    }

    Rectangle {
        visible: icon.featured && icon.hasGlow
        anchors.centerIn: parent
        width: Math.max(parent.width, parent.height) * 1.25
        height: width
        radius: width / 2
        color: icon.glowColor
        opacity: 0.07 + icon.glowStrength * 0.08
    }

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
        x: 0
        y: Math.max(1, icon.height * 0.07)
        width: parent.width
        height: parent.height
        source: sourceImage
        colorization: 1.0
        colorizationColor: "#02060A"
        brightness: -0.35
        saturation: 0.0
        opacity: icon.enabled ? (0.18 + icon.lift * 0.10) : 0.08
    }

    MultiEffect {
        x: 0
        y: -Math.max(0.5, icon.height * 0.025)
        width: parent.width
        height: parent.height
        source: sourceImage
        colorization: 1.0
        colorizationColor: "#FFFFFF"
        brightness: 0.10
        saturation: 0.0
        opacity: icon.enabled ? (icon.featured ? 0.16 : 0.08) : 0.03
    }

    MultiEffect {
        anchors.fill: parent
        source: sourceImage
        colorization: 1.0
        colorizationColor: icon.tint
        brightness: icon.featured ? 0.10 : 0.05
        saturation: 0.0
        opacity: icon.iconOpacity
    }
}
