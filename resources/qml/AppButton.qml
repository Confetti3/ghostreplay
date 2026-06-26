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
    property color accentColor: theme ? theme.accent : "#2563EB"

    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    font: theme ? theme.bodyFont : Qt.font({ pixelSize: 13 })
    implicitHeight: compact ? 32 : 38
    implicitWidth: Math.max(compact ? 34 : 74, contentRow.implicitWidth + leftPadding + rightPadding)
    leftPadding: compact ? 8 : 12
    rightPadding: compact ? 8 : 12
    topPadding: 0
    bottomPadding: 0
    scale: 1.0

    Accessible.role: Accessible.Button
    Accessible.name: text.length > 0 ? text : tooltip
    Accessible.description: detail

    background: GlassPanel {
        theme: control.theme
        tone: "raised"
        radius: 0
        sheenOpacity: 0
        depth: 0
        color: !control.enabled
            ? Qt.rgba(0.04, 0.06, 0.10, 0.35)
            : control.danger
                ? (control.down ? Qt.rgba(control.accentColor.r, 0, 0, 0.22) : control.hovered ? Qt.rgba(control.accentColor.r, 0, 0, 0.15) : Qt.rgba(control.accentColor.r, 0, 0, 0.08))
                : control.primary
                    ? (control.down ? Qt.rgba(control.accentColor.r, control.accentColor.g, control.accentColor.b, 0.22) : control.hovered ? Qt.rgba(control.accentColor.r, control.accentColor.g, control.accentColor.b, 0.15) : Qt.rgba(control.accentColor.r, control.accentColor.g, control.accentColor.b, 0.08))
                    : (control.down ? (control.theme ? control.theme.pressed : "#213054") : control.hovered ? (control.theme ? control.theme.hover : "#17223D") : (control.theme ? control.theme.surfaceRaised : "#111A30"))
        border.color: !control.enabled
            ? (control.theme ? control.theme.border : "#1B2A4A")
            : control.activeFocus
            ? control.accentColor
            : control.danger
                ? (control.theme ? control.theme.danger : "#EF4444")
                : control.primary
                    ? control.accentColor
                    : control.hovered
                        ? (control.theme ? control.theme.borderStrong : "#334E80")
                        : (control.theme ? control.theme.border : "#1B2A4A")
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
                ? (control.theme ? control.theme.textSoft : "#475569")
                : control.danger
                    ? (control.theme ? control.theme.danger : "#EF4444")
                    : control.hovered || control.primary
                        ? (control.accentColor === (control.theme ? control.theme.accent : "#2563EB") || control.accentColor === (control.theme ? control.theme.accentBlue : "#2563EB")
                            ? (control.theme ? control.theme.textPrimary : "#E2E8F0")
                            : control.accentColor)
                        : (control.theme ? control.theme.textSecondary : "#94A3B8")
            glowColor: control.primary && control.enabled
                ? (control.accentColor === (control.theme ? control.theme.accent : "#2563EB") || control.accentColor === (control.theme ? control.theme.accentBlue : "#2563EB")
                    ? (control.theme ? control.theme.textPrimary : control.accentColor)
                    : control.accentColor)
                : "transparent"
            glowStrength: control.primary ? 0.06 : 0.0
            treatment: control.primary ? "featured" : "compact"
            iconOpacity: control.enabled ? 0.92 : 0.35
        }

        Text {
            visible: control.text.length > 0 && !control.compact
            text: control.text
            color: !control.enabled
                ? (control.theme ? control.theme.textSoft : "#475569")
                : control.danger
                    ? (control.theme ? control.theme.danger : "#EF4444")
                    : (control.theme ? control.theme.textPrimary : "#E2E8F0")
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
                : (control.theme ? control.theme.textSoft : "#475569")
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
