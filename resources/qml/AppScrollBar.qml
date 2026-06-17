import QtQuick
import QtQuick.Controls

ScrollBar {
    id: bar

    property QtObject theme

    policy: ScrollBar.AsNeeded
    minimumSize: 0.12

    background: Item {}

    contentItem: Rectangle {
        implicitWidth: 7
        radius: 4
        color: bar.pressed
            ? (bar.theme ? bar.theme.textSecondary : "#D4DEEA")
            : (bar.hovered ? (bar.theme ? bar.theme.textMuted : "#99A8BA") : (bar.theme ? bar.theme.textSoft : "#72839A"))
        opacity: 0.92
    }
}
