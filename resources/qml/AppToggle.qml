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
        radius: height / 2
        color: control.checked
            ? (control.theme ? control.theme.accent : "#8EF7C7")
            : (control.theme ? control.theme.borderStrong : "#496260")
        border.color: control.activeFocus
            ? (control.theme ? control.theme.accentStrong : "#B8FFE0")
            : (control.theme ? control.theme.border : "#243137")
        border.width: control.activeFocus ? 2 : 1

        Rectangle {
            width: 22
            height: 22
            radius: 11
            x: control.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            color: control.theme ? control.theme.textPrimary : "#F5F8FD"
            Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }
    }
}
