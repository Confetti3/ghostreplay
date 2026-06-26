import QtQuick

Item {
    id: logo

    property QtObject theme
    property bool framed: true
    property string source: "qrc:/icons/ghostreplay.svg"
    property real imageMargin: framed ? 2 : 0

    implicitWidth: 28
    implicitHeight: 28

    Rectangle {
        visible: logo.framed
        anchors.centerIn: parent
        width: parent.width + 8
        height: parent.height + 8
        radius: 0
        color: logo.theme ? logo.theme.accentBlue : "#2563EB"
        opacity: 0.12
    }

    GlassPanel {
        anchors.fill: parent
        theme: logo.theme
        tone: "chrome"
        visible: logo.framed
        radius: 0
        border.color: logo.theme ? logo.theme.borderStrong : "#334E80"
        color: logo.theme ? logo.theme.chrome : "#070A13"
    }

    Rectangle {
        visible: logo.framed
        anchors.fill: parent
        radius: 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.22, 0.43, 0.72, 0.22) }
            GradientStop { position: 0.28; color: Qt.rgba(0.10, 0.17, 0.28, 0.10) }
            GradientStop { position: 1.0; color: Qt.rgba(0.0, 0.0, 0.0, 0.0) }
        }
    }

    Image {
        anchors.fill: parent
        anchors.margins: logo.imageMargin
        source: logo.source
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }
}
