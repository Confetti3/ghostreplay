import QtQuick

Rectangle {
    id: panel

    property QtObject theme
    property string tone: "surface"
    property color accentLine: "transparent"
    property real sheenOpacity: 1.0
    property real depth: 1.0

    radius: 0
    color: {
        if (!theme)
            return "rgba(22, 27, 38, 0.45)"
        
        let c = theme.surface
        let alpha = 0.48
        
        if (tone === "chrome") {
            c = theme.chrome
            alpha = 0.78
        } else if (tone === "rail") {
            c = theme.rail
            alpha = 0.48
        } else if (tone === "alt") {
            c = theme.surfaceAlt
            alpha = 0.38
        } else if (tone === "raised") {
            c = theme.surfaceRaised
            alpha = 0.52
        } else if (tone === "strong") {
            c = theme.surfaceStrong
            alpha = 0.72
        }
        
        return Qt.rgba(c.r, c.g, c.b, alpha)
    }
    border.color: theme ? (tone === "chrome" ? theme.borderStrong : theme.border) : "#1B2A4A"
    border.width: 1
    clip: true
    Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        opacity: 0.62 * panel.depth
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.020) }
            GradientStop { position: 0.48; color: Qt.rgba(0.0, 0.0, 0.0, 0.000) }
            GradientStop { position: 1.0; color: Qt.rgba(0.0, 0.0, 0.0, 0.180) }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: panel.theme ? Qt.rgba(1.0, 1.0, 1.0, 0.060 + 0.018 * panel.depth) : "transparent"
        border.width: 1
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        opacity: 0.72 * panel.sheenOpacity
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.060) }
            GradientStop { position: 0.16; color: Qt.rgba(1.0, 1.0, 1.0, 0.016) }
            GradientStop { position: 1.0; color: Qt.rgba(0.0, 0.0, 0.0, 0.0) }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Math.max(2, parent.radius / 2)
        anchors.rightMargin: Math.max(2, parent.radius / 2)
        height: 1
        color: Qt.rgba(1.0, 1.0, 1.0, 0.070 * panel.sheenOpacity)
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: Math.max(4, parent.radius)
        anchors.bottomMargin: Math.max(4, parent.radius)
        width: 1
        color: Qt.rgba(1.0, 1.0, 1.0, 0.038 * panel.sheenOpacity)
    }

    Rectangle {
        visible: panel.accentLine.a > 0
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8
        width: 2
        height: Math.max(18, parent.height - 16)
        radius: 0
        color: panel.accentLine
        opacity: 0.9
    }
}
