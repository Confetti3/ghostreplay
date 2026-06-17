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
    property string targetName: "Desktop"
    property string shortcut: "Alt+F10"
    property bool exportBusy: false
    property real exportProgress: 0
    property string exportStatus: ""
    readonly property bool bufferReady: captureAvailable && bufferSeconds > 0 && bufferPackets > 0
    readonly property real fillRatio: captureAvailable ? Math.max(0, Math.min(1, bufferSeconds / 60)) : 0
    signal saveRequested()
    signal retryRequested()

    tone: "raised"
    theme: strip.theme
    Layout.fillWidth: true
    Layout.preferredHeight: 128
    accentLine: captureAvailable ? (bufferReady ? theme.accent : theme.accentAmber) : theme.danger
    sheenOpacity: 1.05
    depth: 1.16
    border.color: captureAvailable ? (bufferReady ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.42) : Qt.rgba(theme.accentAmber.r, theme.accentAmber.g, theme.accentAmber.b, 0.52)) : theme.dangerSoft
    color: captureAvailable ? Qt.rgba(0.026, 0.052, 0.046, 0.98) : Qt.rgba(0.058, 0.030, 0.034, 0.98)

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        opacity: 0.74
        gradient: Gradient {
            GradientStop { position: 0.0; color: captureAvailable ? Qt.rgba(0.082, 0.150, 0.120, 0.72) : Qt.rgba(0.130, 0.048, 0.052, 0.70) }
            GradientStop { position: 0.44; color: Qt.rgba(0.014, 0.030, 0.028, 0.15) }
            GradientStop { position: 1.0; color: Qt.rgba(0.000, 0.000, 0.000, 0.00) }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.topMargin: 13
        anchors.bottomMargin: 14
        spacing: 9

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.preferredWidth: 118
                Layout.minimumWidth: 118
                Layout.maximumWidth: 118
                spacing: 0

                Text {
                    text: "BUFFER"
                    color: theme.textSoft
                    font: Qt.font({ family: theme.displayFamily, pixelSize: 10, weight: Font.DemiBold })
                    Layout.fillWidth: true
                }

                Text {
                    text: captureAvailable ? bufferSeconds + "s" : "--"
                    color: captureAvailable ? theme.accent : theme.danger
                    font: Qt.font({ family: theme.monoFamily, pixelSize: 34, weight: Font.DemiBold })
                    horizontalAlignment: Text.AlignLeft
                    Layout.fillWidth: true
                }

                Text {
                    text: !captureAvailable ? "NEEDS CHECK" : !recording ? "PAUSED" : bufferReady ? "READY TO SAVE" : "ARMING"
                    color: captureAvailable && bufferReady ? theme.accent : theme.textSoft
                    font: Qt.font({ family: theme.displayFamily, pixelSize: 10, weight: Font.DemiBold })
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: !captureAvailable
                        ? "Capture needs attention"
                        : !recording
                            ? "Replay buffer is paused"
                            : bufferReady ? "Replay buffer is live" : "Buffer is warming up"
                    color: theme.textPrimary
                    font: Qt.font({ family: theme.displayFamily, pixelSize: 23, weight: Font.DemiBold })
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: !captureAvailable
                        ? "Run capture checks, then save new moments with " + shortcut + "."
                        : bufferReady
                            ? bufferSeconds + "s ready from " + targetName + " - " + bufferPackets + " packets in memory"
                            : "Waiting for frames from " + targetName + ". Save will be ready in a moment."
                    color: theme.textMuted
                    font: theme.bodyFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            AppButton {
                theme: strip.theme
                iconSource: captureAvailable ? "qrc:/icons/ui/save.svg" : "qrc:/icons/ui/refresh.svg"
                text: captureAvailable ? "Save replay" : "Retry capture"
                detail: captureAvailable ? shortcut : "Health"
                primary: true
                accentColor: captureAvailable ? theme.accentAmber : theme.danger
                Layout.preferredWidth: captureAvailable ? 176 : 164
                Layout.minimumWidth: Layout.preferredWidth
                enabled: !captureAvailable || bufferReady
                onClicked: captureAvailable ? strip.saveRequested() : strip.retryRequested()
            }
        }

        Item {
            id: track
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            clip: true

            Rectangle {
                id: rail
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 20
                radius: 10
                color: Qt.rgba(0.010, 0.018, 0.017, 0.98)
                border.color: captureAvailable ? Qt.rgba(0.56, 0.97, 0.78, bufferReady ? 0.20 : 0.13) : theme.dangerSoft
                border.width: 1
            }

            Rectangle {
                anchors.left: rail.left
                anchors.right: rail.right
                anchors.top: rail.top
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                anchors.topMargin: 1
                height: 1
                color: Qt.rgba(1.0, 1.0, 1.0, 0.075)
            }

            Rectangle {
                id: fill
                anchors.left: rail.left
                anchors.verticalCenter: rail.verticalCenter
                width: rail.width * strip.fillRatio
                height: 18
                radius: 9
                color: "transparent"
                opacity: captureAvailable ? 1.0 : 0.0
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: bufferReady ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.34) : Qt.rgba(theme.accentAmber.r, theme.accentAmber.g, theme.accentAmber.b, 0.18) }
                    GradientStop { position: 1.0; color: bufferReady ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.13) : Qt.rgba(theme.accentAmber.r, theme.accentAmber.g, theme.accentAmber.b, 0.08) }
                }
                Behavior on width { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
            }

            Rectangle {
                id: sweep
                visible: captureAvailable && recording && bufferReady
                width: Math.max(90, rail.width * 0.13)
                height: 18
                radius: 9
                y: rail.y + 1
                opacity: visible ? 0.30 : 0.0
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.00) }
                    GradientStop { position: 0.48; color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.22) }
                    GradientStop { position: 1.0; color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.00) }
                }

                NumberAnimation on x {
                    running: sweep.visible
                    loops: Animation.Infinite
                    from: -sweep.width
                    to: rail.width
                    duration: 2200
                    easing.type: Easing.InOutQuad
                }
            }

            Repeater {
                model: 37
                Rectangle {
                    width: index % 6 === 0 ? 2 : 1
                    height: index % 6 === 0 ? 30 : 19
                    radius: 1
                    x: rail.x + rail.width * index / Math.max(1, 36) - width / 2
                    anchors.verticalCenter: rail.verticalCenter
                    readonly property real chargedIndex: Math.min(36, Math.max(0, bufferSeconds / 60 * 36))
                    color: captureAvailable
                        ? (index >= 33 ? theme.accentAmber : (recording && index <= chargedIndex ? theme.accent : theme.textSoft))
                        : theme.danger
                    opacity: captureAvailable ? (index <= chargedIndex ? 0.46 + Math.min(0.28, bufferSeconds / 110) : 0.18) : 0.28
                    Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                }
            }

            Rectangle {
                id: nowStem
                width: 2
                height: 30
                radius: 1
                anchors.verticalCenter: rail.verticalCenter
                x: rail.x + rail.width - width
                color: captureAvailable ? theme.accentAmber : theme.danger
                opacity: 0.86
            }

            Rectangle {
                id: nowMarker
                width: 20
                height: 20
                radius: 10
                anchors.verticalCenter: rail.verticalCenter
                x: Math.max(0, rail.x + rail.width - width)
                color: captureAvailable ? theme.accentAmber : theme.danger
                border.color: Qt.rgba(theme.textPrimary.r, theme.textPrimary.g, theme.textPrimary.b, 0.94)
                border.width: 1
                Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
            }

            Rectangle {
                visible: exportBusy
                anchors.left: rail.left
                anchors.verticalCenter: rail.verticalCenter
                width: rail.width * Math.max(0, Math.min(1, exportProgress))
                height: 5
                radius: 2
                color: theme.accentPurple
                Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            }

            Text {
                anchors.right: rail.right
                anchors.top: parent.top
                anchors.topMargin: -2
                text: exportBusy ? (exportStatus.length > 0 ? exportStatus : "Exporting") : "now"
                color: exportBusy ? theme.accentPurple : theme.textSoft
                font: theme.smallFont
            }

        }
    }
}
