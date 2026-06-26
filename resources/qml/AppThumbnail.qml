import QtQuick
import QtQuick.Controls

Rectangle {
    id: thumbnail

    property QtObject theme
    property string clipPath: ""
    property string fallbackText: "No preview"
    property real cornerRadius: theme ? theme.radiusSm : 0
    property bool hoverActive: false

    radius: cornerRadius
    color: theme ? theme.overlay : "#050811"
    border.color: theme ? theme.border : "#1B2A4A"
    border.width: 1
    clip: true

    Image {
        id: thumb
        anchors.fill: parent
        anchors.margins: 1
        source: thumbnail.clipPath.length > 0
            ? "image://thumbnails/" + encodeURIComponent(thumbnail.clipPath)
            : ""
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
        visible: status === Image.Ready
        scale: thumbnail.hoverActive ? 1.05 : 1.0
        Behavior on scale { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: thumb.status === Image.Loading
        visible: running
    }

    Text {
        anchors.centerIn: parent
        width: Math.max(44, parent.width - 14)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: thumb.status === Image.Error ? "Preview failed" : thumbnail.fallbackText
        visible: thumbnail.clipPath.length === 0 || thumb.status === Image.Error || thumb.status === Image.Null
        color: thumbnail.theme ? thumbnail.theme.textSoft : "#475569"
        font: thumbnail.theme ? thumbnail.theme.smallFont : Qt.font({ pixelSize: 11 })
        elide: Text.ElideRight
    }
}
