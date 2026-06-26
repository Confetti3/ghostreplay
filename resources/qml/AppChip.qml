import QtQuick
import QtQuick.Layouts

Rectangle {
    id: chip

    property QtObject theme
    property string text: ""
    property color accentColor: theme ? theme.accent : "#2563EB"
    property bool strong: false

    function tintedSurface(alphaBoost) {
        return Qt.rgba(
            chip.accentColor.r,
            chip.accentColor.g,
            chip.accentColor.b,
            alphaBoost
        )
    }

    radius: 0
    color: strong ? tintedSurface(0.12) : tintedSurface(0.06)
    border.color: strong ? accentColor : (theme ? theme.border : "#1B2A4A")
    border.width: 1
    implicitHeight: 26
    implicitWidth: label.implicitWidth + 28
    Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

    RowLayout {
        anchors.centerIn: parent
        spacing: 7

        Rectangle {
            id: statusDot
            Layout.preferredWidth: 7
            Layout.preferredHeight: 7
            radius: 0
            color: chip.accentColor
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.18)
            border.width: 1

            SequentialAnimation on opacity {
                running: chip.strong
                loops: Animation.Infinite
                NumberAnimation { to: 0.62; duration: 1100; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 1.0; duration: 1100; easing.type: Easing.InOutQuad }
            }
        }

        Text {
            id: label
            text: chip.text
            color: chip.theme ? chip.theme.textPrimary : "#E2E8F0"
            font: chip.theme ? chip.theme.smallFont : Qt.font({ pixelSize: 11 })
        }
    }
}
