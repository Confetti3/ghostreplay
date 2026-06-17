import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page

    background: Rectangle { color: "transparent" }
    property var overviewClips: []

    RowLayout {
        anchors.fill: parent
        spacing: 14

        Item {
            id: mainPane
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 13

                OverviewHeader {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                }

                GlassPanel {
                    visible: !backend.captureAvailable
                    theme: root.uiTheme
                    tone: "raised"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    border.color: Qt.rgba(1.0, 0.52, 0.52, 0.40)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 14

                        Rectangle {
                            Layout.preferredWidth: 42
                            Layout.preferredHeight: 42
                            radius: 21
                            color: Qt.rgba(1.0, 0.48, 0.48, 0.11)
                            border.color: root.recordRed
                            border.width: 1

                            MonoIcon {
                                anchors.centerIn: parent
                                source: root.iconVideo
                                width: 18
                                height: 18
                                tint: root.recordRed
                                glowColor: root.recordRed
                                glowStrength: 0.10
                                treatment: "featured"
                                iconOpacity: 0.96
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Text {
                                text: "Capture unavailable"
                                color: root.textPrimary
                                font: root.titleFont
                            }

                            Text {
                                text: backend.captureError || "Recording requires Windows desktop capture and a supported video encoder."
                                color: root.textSecondary
                                font: root.smallFont
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }

                        TextButton { text: "Retry"; iconPath: root.iconRefresh; onClicked: backend.retryCapture() }
                        TextButton { text: "Log"; iconPath: root.iconOpen; onClicked: backend.openLogFile() }
                        TextButton { text: "Diagnostics"; iconPath: root.iconOutput; onClicked: backend.copyDiagnostics() }
                    }
                }

                ReplayBufferStrip {
                    theme: root.uiTheme
                    captureAvailable: backend.captureAvailable
                    recording: backend.recording
                    bufferSeconds: backend.bufferSeconds
                    bufferPackets: backend.bufferPackets
                    targetName: backend.currentGame || "Desktop"
                    shortcut: root.shortcutText()
                    exportBusy: backend.exportBusy
                    exportProgress: backend.exportProgress
                    exportStatus: backend.exportStatus
                    onSaveRequested: backend.saveClip()
                    onRetryRequested: backend.retryCapture()
                    Layout.preferredHeight: 128
                }

                SessionSummary {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 88
                }

                GlassPanel {
                    id: recentSurface
                    theme: root.uiTheme
                    tone: "raised"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sheenOpacity: 0.86
                    depth: 0.95
                    color: Qt.rgba(0.038, 0.055, 0.058, 0.94)
                    border.color: Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.74)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        anchors.topMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Text {
                                    text: "Recent saves"
                                    color: root.textPrimary
                                    font: root.titleFont
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: totalClipCount() > overviewClips.length
                                        ? "Latest " + overviewClips.length + " of " + totalClipCount() + " saved clips"
                                        : overviewClips.length + (overviewClips.length === 1 ? " saved clip" : " saved clips") + " ready to open or share"
                                    color: root.textSoft
                                    font: root.smallFont
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Item { Layout.fillWidth: true }

                            TextButton {
                                text: "All clips"
                                iconPath: root.iconGrid
                                Layout.preferredWidth: 104
                                onClicked: root.goToView(1, true)
                            }
                        }

                        ListView {
                            id: recentList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            readonly property int scrollGutter: 20
                            clip: true
                            spacing: 7
                            model: overviewClips
                            boundsBehavior: Flickable.StopAtBounds

                            Rectangle {
                                anchors.fill: parent
                                color: "transparent"
                                z: -2
                            }

                            delegate: CompactClipRow {
                                width: Math.max(0, recentList.width - recentList.scrollGutter)
                                clipData: modelData
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: recentList.count === 0
                                text: "No clips saved yet. Press " + root.shortcutText() + " after a moment you want to keep."
                                color: root.textSoft
                                font: root.bodyFont
                                width: Math.min(parent.width - 48, 420)
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                            }

                            ScrollBar.vertical: AppScrollBar {
                                theme: root.uiTheme
                                policy: recentList.contentHeight > recentList.height + 1 ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                                x: recentList.width - width - 4
                            }
                        }

                    }
                }
            }
        }

        GlassPanel {
            theme: root.uiTheme
            tone: "rail"
            Layout.preferredWidth: page.width >= 1120 ? 300 : 0
            Layout.minimumWidth: page.width >= 1120 ? 300 : 0
            Layout.maximumWidth: page.width >= 1120 ? 300 : 0
            Layout.fillHeight: true
            visible: page.width >= 1120

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.topMargin: 18
                anchors.bottomMargin: 16
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "Capture health"
                        color: root.textPrimary
                        font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 18, weight: Font.DemiBold })
                    }

                    Text {
                        text: backend.captureAvailable ? "Recording status and output readiness." : "Capture needs attention before new clips can be saved."
                        color: root.textSoft
                        font: root.smallFont
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                InspectorSection {
                    title: "Pipeline"
                    rows: [
                        { label: "Capture", value: backend.captureAvailable ? (backend.recording ? "DXGI active" : "Paused") : "Unavailable", accent: backend.captureAvailable ? (backend.recording ? root.successGreen : root.inactive) : root.recordRed },
                        { label: "Encode", value: backend.captureCodec.toUpperCase() + " preset " + backend.capturePreset, accent: root.successGreen },
                        { label: "Buffer", value: bufferReady() ? backend.bufferSeconds + "s rolling window" : "Warming up", accent: bufferReady() ? root.accent : root.warningYellow },
                        { label: "Save", value: "MP4 export on demand", accent: root.textSecondary }
                    ]
                }

                InspectorSection {
                    title: "Session"
                    rows: [
                        { label: "Target", value: backend.currentGame || "Desktop", accent: root.successGreen },
                        { label: "Audio", value: backend.audioActive ? "WASAPI loopback" : "Disabled", accent: backend.audioActive ? root.successGreen : root.inactive },
                        { label: "Hotkey", value: root.shortcutText(), accent: root.accent },
                        { label: "Output", value: "Clips", accent: root.textSecondary }
                    ]
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "Workflow"
                        color: root.textMuted
                        font: root.smallFont
                    }

                    StepRow { indexText: "1"; detail: "Keep capture live while you play or work." }
                    StepRow { indexText: "2"; detail: "Press " + root.shortcutText() + " after the moment lands." }
                    StepRow { indexText: "3"; detail: "Share or reveal it from Clips." }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "Ghost Replay"
                        color: root.textSoft
                        font: root.smallFont
                    }

                    Rectangle { width: 3; height: 3; radius: 2; color: root.textSoft; opacity: 0.7 }

                    Text {
                        text: "v" + backend.appVersion
                        color: root.textSoft
                        font: root.smallFont
                    }
                }
            }
        }
    }

    function videoProfile() {
        let res = backend.captureWidth > 0 && backend.captureHeight > 0
            ? backend.captureWidth + "x" + backend.captureHeight
            : "Native"
        return res + " " + backend.captureFps
    }

    function bufferReady() {
        return backend.captureAvailable && backend.bufferSeconds > 0 && backend.bufferPackets > 0
    }

    function updateOverviewClips() {
        let source = backend.clipLibrary && backend.clipLibrary.length > 0 ? backend.clipLibrary : backend.recentClips
        let clips = []
        for (let i = 0; i < source.length && i < 3; ++i)
            clips.push(source[i])
        overviewClips = clips
    }

    function totalClipCount() {
        let source = backend.clipLibrary && backend.clipLibrary.length > 0 ? backend.clipLibrary : backend.recentClips
        return source ? source.length : 0
    }

    function clipSubtitle(clip) {
        let parts = []
        if (clip.date)
            parts.push(clip.date)
        else if (clip.timestamp)
            parts.push(clip.timestamp)
        if (clip.duration)
            parts.push(clip.duration)
        if (clip.size)
            parts.push(clip.size)
        return parts.join("   ")
    }

    Component.onCompleted: {
        backend.requestRefresh()
        updateOverviewClips()
    }

    onVisibleChanged: if (visible) {
        backend.requestRefresh()
        updateOverviewClips()
    }

    Connections {
        target: backend
        function onClipLibraryChanged() { updateOverviewClips() }
        function onRecentClipsChanged() { updateOverviewClips() }
    }

    component OverviewHeader: Item {
        RowLayout {
            anchors.fill: parent
            spacing: 12

            GlassPanel {
                theme: root.uiTheme
                tone: "raised"
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50
                radius: 14
                sheenOpacity: 1.12
                depth: 1.18
                color: Qt.rgba(0.055, 0.095, 0.115, 0.92)
                border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.28)

                MonoIcon {
                    anchors.centerIn: parent
                    source: root.iconOverview
                    width: 24
                    height: 24
                    tint: root.accent
                    glowColor: root.accent
                    glowStrength: 0.13
                    treatment: "featured"
                    iconOpacity: 0.98
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Text {
                    text: "Overview"
                    color: root.textPrimary
                    font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 24, weight: Font.DemiBold })
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: backend.captureAvailable
                        ? (backend.recording ? "Capture is running. Save the last moment when something worth keeping happens." : "Capture is paused. Resume to rebuild the replay buffer.")
                        : "Capture needs attention. Existing clips, logs, and diagnostics are still available."
                    color: root.textMuted
                    font: root.bodyFont
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                AppChip {
                    theme: root.uiTheme
                    text: backend.captureAvailable ? (backend.recording ? "Recording" : "Paused") : "Unavailable"
                    accentColor: backend.captureAvailable ? (backend.recording ? root.accent : root.warningYellow) : root.recordRed
                    strong: true
                }

                AppChip {
                    visible: mainPane.width >= 760
                    theme: root.uiTheme
                    text: root.shortcutText()
                    accentColor: root.accentAmber
                    strong: true
                }
            }
        }
    }

    component SessionSummary: GlassPanel {
        id: summary

        theme: root.uiTheme
        tone: "surface"
        sheenOpacity: 0.80
        depth: 0.92
        color: Qt.rgba(0.038, 0.056, 0.060, 0.92)
        border.color: Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.58)

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 0

            StatusCell {
                icon: root.iconVideo
                label: "Capture"
                value: backend.captureAvailable ? (backend.recording ? "Live" : "Paused") : "Unavailable"
                detail: backend.captureAvailable ? (backend.currentGame || "Desktop") : "Retry from settings"
                accentColor: backend.captureAvailable ? root.accentBlue : root.recordRed
            }

            SummaryDivider {}

            StatusCell {
                icon: root.iconBuffer
                label: "Buffer"
                value: bufferReady() ? backend.bufferSeconds + "s ready" : "Warming up"
                detail: bufferReady() ? backend.bufferPackets + " packets in memory" : "Waiting for packets"
                accentColor: bufferReady() ? root.accent : root.warningYellow
            }

            SummaryDivider {}

            StatusCell {
                icon: root.iconOutput
                label: "Video"
                value: videoProfile()
                detail: backend.captureCodec.toUpperCase() + "  CQP " + backend.captureCqp
                accentColor: root.accentPurple
            }

            SummaryDivider {}

            StatusCell {
                icon: root.iconAudio
                label: "Audio"
                value: backend.audioActive ? "Enabled" : "Disabled"
                detail: backend.audioActive ? backend.audioSampleRate + " Hz stereo" : "Loopback inactive"
                accentColor: backend.audioActive ? root.warningYellow : root.inactive
            }
        }
    }

    component SummaryDivider: Rectangle {
        Layout.preferredWidth: 1
        Layout.fillHeight: true
        Layout.topMargin: 18
        Layout.bottomMargin: 18
        color: Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.42)
    }

    component StatusCell: Item {
        id: statusCell

        property string icon
        property string label
        property string value
        property string detail
        property color accentColor: root.accent

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 142

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            MonoIcon {
                source: statusCell.icon
                Layout.preferredWidth: 19
                Layout.preferredHeight: 19
                tint: statusCell.accentColor
                glowColor: statusCell.accentColor
                glowStrength: 0.10
                treatment: "featured"
                iconOpacity: 0.95
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: statusCell.label
                    color: root.textMuted
                    font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 10, weight: Font.DemiBold })
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: statusCell.value
                    color: root.textPrimary
                    font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 19, weight: Font.DemiBold })
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: statusCell.detail
                    color: root.textMuted
                    font: root.smallFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }

    component CompactClipRow: GlassPanel {
        id: clipRow

        property var clipData

        theme: root.uiTheme
        tone: "surface"
        height: 58
        sheenOpacity: rowMouse.containsMouse ? 1.0 : 0.70
        depth: rowMouse.containsMouse ? 1.04 : 0.82
        color: rowMouse.pressed ? root.pressedBg : rowMouse.containsMouse ? Qt.rgba(0.075, 0.115, 0.125, 0.92) : Qt.rgba(0.025, 0.038, 0.042, 0.78)
        border.color: rowMouse.containsMouse ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.35) : Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.64)

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 8
            spacing: 12

            AppThumbnail {
                theme: root.uiTheme
                Layout.preferredWidth: 78
                Layout.preferredHeight: 42
                clipPath: clipRow.clipData ? clipRow.clipData.path : ""
                fallbackText: "No preview"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Text {
                    text: clipRow.clipData ? clipRow.clipData.name : ""
                    color: root.textPrimary
                    font: root.bodyFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: clipRow.clipData ? clipSubtitle(clipRow.clipData) : ""
                    color: root.textSoft
                    font: root.smallFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            TextButton {
                text: "Share"
                iconPath: root.iconEdit
                Layout.preferredWidth: 92
                onClicked: if (clipRow.clipData) root.openClipEditor(clipRow.clipData)
            }

            MiniIconButton {
                icon: root.iconOpen
                tooltip: "Open clip"
                onClicked: if (clipRow.clipData) backend.openClip(clipRow.clipData.path)
            }
        }

        MouseArea {
            id: rowMouse
            z: -1
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
        }

        Accessible.role: Accessible.Button
        Accessible.name: clipRow.clipData ? clipRow.clipData.name : "Recent clip"
        Accessible.description: clipRow.clipData ? clipSubtitle(clipRow.clipData) : ""
    }

    component TextButton: AppButton {
        property string iconPath: ""
        theme: root.uiTheme
        iconSource: iconPath
        accentColor: root.controlAccent(iconPath, false)
        implicitHeight: 32
    }

    component MiniIconButton: GlassPanel {
        id: miniIconButton

        property string icon
        property string tooltip
        property color accentColor: root.controlAccent(icon, false)
        signal clicked()

        theme: root.uiTheme
        tone: "raised"
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32
        color: miniMouse.pressed ? root.pressedBg : miniMouse.containsMouse ? root.hoverBg : root.panelRaised
        border.color: miniMouse.containsMouse ? root.borderStrong : root.border

        MonoIcon {
            anchors.centerIn: parent
            source: miniIconButton.icon
            width: 14
            height: 14
            tint: miniMouse.containsMouse ? miniIconButton.accentColor : root.textSecondary
            glowColor: "transparent"
            glowStrength: 0.0
            treatment: "compact"
            iconOpacity: miniMouse.containsMouse ? 0.92 : 0.78
        }

        ToolTip.visible: miniMouse.containsMouse && miniIconButton.tooltip.length > 0
        ToolTip.text: miniIconButton.tooltip
        Accessible.role: Accessible.Button
        Accessible.name: miniIconButton.tooltip

        MouseArea {
            id: miniMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: miniIconButton.clicked()
        }
    }

    component InspectorSection: Item {
        id: inspectorSection

        property string title
        property var rows

        Layout.fillWidth: true
        implicitHeight: inspectorContent.implicitHeight

        ColumnLayout {
            id: inspectorContent
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 8

            Text {
                text: inspectorSection.title
                color: root.textMuted
                font: root.smallFont
            }

            Repeater {
                model: inspectorSection.rows
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 9

                    Rectangle {
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        radius: 4
                        color: modelData.accent
                    }

                    Text {
                        text: modelData.label
                        color: root.textSoft
                        font: root.smallFont
                        Layout.preferredWidth: 58
                    }

                    Text {
                        text: modelData.value
                        color: root.textPrimary
                        font: root.smallFont
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }
                }
            }
        }
    }

    component StepRow: RowLayout {
        property string indexText
        property string detail

        Layout.fillWidth: true
        spacing: 9

        GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            Layout.preferredWidth: 22
            Layout.preferredHeight: 22
            radius: 11
            border.color: root.borderStrong

            Text {
                anchors.centerIn: parent
                text: parent.parent.indexText
                color: root.textSecondary
                font: root.smallFont
            }
        }

        Text {
            text: parent.detail
            color: root.textSecondary
            font: root.smallFont
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            lineHeight: 1.08
        }
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: root.border
    }
}
