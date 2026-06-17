import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control

    property QtObject theme
    property string iconSource: ""
    property string tooltip: ""
    property string detail: ""
    property bool primary: false
    property bool danger: false
    property bool compact: false
    property color accentColor: theme ? theme.accent : "#8EF7C7"

    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    font: theme ? theme.bodyFont : Qt.font({ pixelSize: 13 })
    implicitHeight: compact ? 32 : 38
    implicitWidth: Math.max(compact ? 34 : 74, contentRow.implicitWidth + leftPadding + rightPadding)
    leftPadding: compact ? 8 : 12
    rightPadding: compact ? 8 : 12
    topPadding: 0
    bottomPadding: 0
    scale: !enabled ? 1.0 : down ? 0.985 : hovered ? 1.006 : 1.0
    Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

    Accessible.role: Accessible.Button
    Accessible.name: text.length > 0 ? text : tooltip
    Accessible.description: detail

    background: GlassPanel {
        theme: control.theme
        tone: "raised"
        radius: control.theme ? control.theme.radiusSm : 8
        sheenOpacity: control.enabled ? (control.hovered || control.primary ? 1.12 : 0.90) : 0.42
        depth: control.enabled ? (control.hovered || control.primary ? 1.12 : 0.88) : 0.55
        color: !control.enabled
            ? Qt.rgba(0.04, 0.05, 0.05, 0.58)
            : control.danger
                ? (control.down ? Qt.rgba(0.31, 0.08, 0.10, 0.94) : Qt.rgba(0.18, 0.07, 0.08, 0.82))
                : control.primary
                    ? (control.down ? Qt.rgba(0.10, 0.25, 0.20, 0.96) : Qt.rgba(0.08, 0.20, 0.17, 0.90))
                    : (control.down ? (control.theme ? control.theme.pressed : "#223238") : control.hovered ? (control.theme ? control.theme.hover : "#1A272D") : (control.theme ? control.theme.surfaceRaised : "#141D22"))
        border.color: !control.enabled
            ? (control.theme ? Qt.rgba(control.theme.border.r, control.theme.border.g, control.theme.border.b, 0.62) : "#243137")
            : control.activeFocus
            ? control.accentColor
            : control.danger
                ? (control.theme ? control.theme.danger : "#F05D5E")
                : control.primary
                    ? control.accentColor
                    : control.hovered
                        ? (control.theme ? control.theme.borderStrong : "#496260")
                        : (control.theme ? control.theme.border : "#243137")
        border.width: control.activeFocus ? 2 : 1
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: control.text.length > 0 && control.iconSource.length > 0 ? 7 : 0

        MonoIcon {
            visible: control.iconSource.length > 0
            source: control.iconSource
            Layout.preferredWidth: control.compact ? 14 : 15
            Layout.preferredHeight: control.compact ? 14 : 15
            tint: !control.enabled
                ? (control.theme ? control.theme.textSoft : "#74849A")
                : control.danger
                    ? (control.theme ? control.theme.danger : "#F05D5E")
                    : control.hovered || control.primary
                        ? control.accentColor
                        : (control.theme ? control.theme.textSecondary : "#D3DDEA")
            glowColor: control.primary && control.enabled ? control.accentColor : "transparent"
            glowStrength: control.primary ? 0.10 : 0.0
            treatment: control.primary ? "featured" : "compact"
            iconOpacity: control.enabled ? 0.92 : 0.35
        }

        Text {
            visible: control.text.length > 0 && !control.compact
            text: control.text
            color: !control.enabled
                ? (control.theme ? control.theme.textSoft : "#74849A")
                : control.danger
                    ? (control.theme ? control.theme.danger : "#F05D5E")
                    : (control.theme ? control.theme.textPrimary : "#F5F8FD")
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            Layout.maximumWidth: 150
        }

        Text {
            visible: control.detail.length > 0 && !control.compact
            text: control.detail
            color: control.primary
                ? control.accentColor
                : (control.theme ? control.theme.textSoft : "#74849A")
            font: control.theme ? control.theme.smallFont : Qt.font({ pixelSize: 11 })
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            Layout.maximumWidth: 84
        }
    }

    ToolTip.visible: hovered && tooltip.length > 0
    ToolTip.delay: 450
    ToolTip.text: tooltip
}
