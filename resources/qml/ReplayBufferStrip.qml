import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: strip

    property QtObject theme
    property bool captureAvailable: true
    property bool recording: true
    property int bufferSeconds: 0
    property int bufferPackets: 0
    property int targetDurationSec: 60
    property string targetName: "Desktop"
    property string shortcut: "Alt+F10"
    property bool exportBusy: false
    property real exportProgress: 0
    property string exportStatus: ""

    readonly property bool bufferReady: captureAvailable && bufferSeconds > 0 && bufferPackets > 0
    readonly property bool arming: captureAvailable && recording && !bufferReady
    readonly property color stateColor: !captureAvailable
        ? theme.danger
        : (bufferReady ? theme.accent : (recording ? theme.accentAmber : theme.inactive))
    property real armingPulse: 0.40
    readonly property real fillRatio: captureAvailable && targetDurationSec > 0
        ? Math.max(0, Math.min(1, bufferSeconds / targetDurationSec))
        : 0
    readonly property string headlineText: !captureAvailable
        ? "Capture needs attention"
        : !recording
            ? "Replay buffer is paused"
            : bufferReady
                ? "Replay buffer is live"
                : "Buffer is warming up"
    readonly property string subtitleText: !captureAvailable
        ? "Run checks, then save with " + shortcut
        : !recording
            ? "Resume capture to rebuild the window"
            : bufferReady
                ? bufferSeconds + "s of " + targetDurationSec + "s from " + targetName
                : "Waiting for frames from " + targetName
    readonly property string statusLabel: !captureAvailable
        ? "Needs check"
        : !recording
            ? "Paused"
            : bufferReady
                ? "Ready"
                : "Arming"

    signal saveRequested()
    signal retryRequested()

    tone: "raised"
    theme: strip.theme
    Layout.fillWidth: true
    Layout.preferredHeight: 108
    radius: 0
    accentLine: "transparent"
    sheenOpacity: 0
    depth: 0
    border.color: theme ? (captureAvailable ? (bufferReady ? theme.borderStrong : theme.border) : theme.danger) : "#1B2A4A"
    color: theme ? (captureAvailable ? Qt.rgba(theme.surfaceRaised.r, theme.surfaceRaised.g, theme.surfaceRaised.b, 0.52) : Qt.rgba(theme.danger.r, 0, 0, 0.12)) : "rgba(17, 26, 48, 0.52)"

    SequentialAnimation on armingPulse {
        running: arming
        loops: Animation.Infinite
        NumberAnimation { to: 0.55; duration: 900; easing.type: Easing.InOutQuad }
        NumberAnimation { to: 0.35; duration: 900; easing.type: Easing.InOutQuad }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        opacity: 0.72
        gradient: Gradient {
            GradientStop { position: 0.0; color: captureAvailable ? Qt.rgba(0.082, 0.150, 0.120, 0.72) : Qt.rgba(0.130, 0.048, 0.052, 0.70) }
            GradientStop { position: 0.44; color: Qt.rgba(0.014, 0.030, 0.028, 0.15) }
            GradientStop { position: 1.0; color: Qt.rgba(0.000, 0.000, 0.000, 0.00) }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 14

        RowLayout {
            spacing: 10
            Layout.preferredWidth: 108
            Layout.maximumWidth: 108

            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: 0
                color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b, 0.08)
                border.color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b, 0.28)
                border.width: 1

                MonoIcon {
                    anchors.centerIn: parent
                    source: "qrc:/icons/ui/buffer.svg"
                    width: 18
                    height: 18
                    tint: stateColor === theme.accent ? theme.textPrimary : Qt.lighter(stateColor, 1.35)
                    glowColor: stateColor === theme.accent ? theme.textPrimary : stateColor
                    glowStrength: stateColor === theme.accent ? 0.22 : 0.12
                    treatment: "plain"
                    iconOpacity: 0.96
                }
            }

            ColumnLayout {
                spacing: 0

                Text {
                    text: captureAvailable ? bufferSeconds + "s" : "--"
                    color: captureAvailable ? theme.accent : theme.danger
                    font: Qt.font({ family: theme.monoFamily, pixelSize: 24, weight: Font.DemiBold })
                }

                Text {
                    text: statusLabel
                    color: bufferReady ? theme.accent : theme.textSoft
                    font: Qt.font({ family: theme.displayFamily, pixelSize: 10, weight: Font.DemiBold })
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: headlineText
                color: theme.textPrimary
                font: Qt.font({ family: theme.displayFamily, pixelSize: 18, weight: Font.DemiBold })
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: subtitleText
                color: theme.textMuted
                font: theme.smallFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Item {
                id: capsuleTrack
                Layout.fillWidth: true
                Layout.preferredHeight: 36

                Text {
                    visible: !exportBusy
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.top: capsuleRail.bottom
                    anchors.topMargin: 4
                    text: "0"
                    color: theme.textSoft
                    font: theme.smallFont
                }

                Text {
                    visible: !exportBusy
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.top: capsuleRail.bottom
                    anchors.topMargin: 4
                    text: targetDurationSec + "s"
                    color: theme.textSoft
                    font: theme.smallFont
                }

                Text {
                    visible: exportBusy
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: capsuleRail.bottom
                    anchors.topMargin: 4
                    text: exportStatus.length > 0 ? exportStatus : "Exporting"
                    color: theme.accentPurple
                    font: theme.smallFont
                    elide: Text.ElideRight
                    width: Math.min(parent.width, implicitWidth)
                }

                Rectangle {
                    id: capsuleRail
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.top: parent.top
                    anchors.topMargin: 8
                    height: 4
                    radius: 0
                    color: theme ? theme.border : "#1B2A4A"
                    border.width: 0

                    MouseArea {
                        id: trackMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }

                    Rectangle {
                        id: capsuleFill
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * strip.fillRatio
                        height: 4
                        radius: 0
                        color: theme ? (bufferReady ? theme.accent : theme.inactive) : "#2563EB"
                        opacity: captureAvailable ? 1.0 : 0.0
                        Behavior on width { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
                    }

                    Rectangle {
                        visible: exportBusy
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * Math.max(0, Math.min(1, exportProgress))
                        height: 4
                        radius: 0
                        color: theme ? theme.accentPurple : "#8B5CF6"
                        Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    }

                    Rectangle {
                        id: nowMarker
                        visible: captureAvailable && !exportBusy
                        width: 4
                        height: 12
                        radius: 0
                        anchors.verticalCenter: parent.verticalCenter
                        x: Math.max(0, Math.min(capsuleRail.width - width, capsuleFill.width - width / 2))
                        color: stateColor
                        border.width: 0
                        Behavior on x { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
                    }
                }
            }
        }

        AppButton {
            theme: strip.theme
            iconSource: captureAvailable ? "qrc:/icons/ui/save.svg" : "qrc:/icons/ui/refresh.svg"
            text: captureAvailable ? "Save replay" : "Retry capture"
            detail: captureAvailable ? shortcut : "Health"
            primary: true
            accentColor: captureAvailable ? theme.accent : theme.danger
            Layout.preferredWidth: captureAvailable ? 168 : 156
            Layout.minimumWidth: 120
            enabled: !captureAvailable || bufferReady
            onClicked: captureAvailable ? strip.saveRequested() : strip.retryRequested()
        }
    }
}
