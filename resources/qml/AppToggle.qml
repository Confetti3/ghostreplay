import QtQuick
import QtQuick.Controls

Switch {
    id: control

    property QtObject theme

    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.CheckBox
    Accessible.name: text

    indicator: Rectangle {
        implicitWidth: 50
        implicitHeight: 28
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 0
        color: control.checked
            ? (control.theme ? control.theme.accent : "#2563EB")
            : (control.theme ? control.theme.borderStrong : "#334E80")
        border.color: control.activeFocus
            ? (control.theme ? control.theme.accentStrong : "#3B82F6")
            : (control.theme ? control.theme.border : "#1B2A4A")
        border.width: control.activeFocus ? 2 : 1

        Rectangle {
            width: 22
            height: 22
            radius: 0
            x: control.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            color: control.theme ? control.theme.textPrimary : "#E2E8F0"
            Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }
    }
}
