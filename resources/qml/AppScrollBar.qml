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
        radius: 0
        color: bar.pressed
            ? (bar.theme ? bar.theme.textSecondary : "#94A3B8")
            : (bar.hovered ? (bar.theme ? bar.theme.textMuted : "#64748B") : (bar.theme ? bar.theme.textSoft : "#475569"))
        opacity: 0.92
    }
}
