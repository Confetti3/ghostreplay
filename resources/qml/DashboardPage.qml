import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page

    background: Rectangle { color: "transparent" }
    property var overviewClips: []

    // Entrance animation properties
    property real appearOffset: 25
    property real appearOpacity: 0
    NumberAnimation on appearOpacity { to: 1.0; duration: 800; easing.type: Easing.OutExpo }
    NumberAnimation on appearOffset { to: 0; duration: 800; easing.type: Easing.OutExpo }

    RowLayout {
        anchors.fill: parent
        spacing: 20

        Item {
            id: mainPane
            Layout.fillWidth: true
            Layout.fillHeight: true
            opacity: page.appearOpacity
            transform: Translate { y: page.appearOffset }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 24

                OverviewHeader {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
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
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 46
                            radius: 23
                            color: Qt.rgba(1.0, 0.48, 0.48, 0.11)
                            border.color: root.recordRed
                            border.width: 1

                            MonoIcon {
                                anchors.centerIn: parent
                                source: root.iconVideo
                                width: 20
                                height: 20
                                tint: root.recordRed
                                glowColor: root.recordRed
                                glowStrength: 0.20
                                treatment: "featured"
                                iconOpacity: 0.96
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Capture unavailable"
                                color: root.textPrimary
                                font: root.headingFont
                            }

                            Text {
                                text: backend.captureError || "Recording requires Windows desktop capture and a supported video encoder."
                                color: root.textSecondary
                                font: root.bodyFont
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

                GridLayout {
                    Layout.fillWidth: true
                    columns: mainPane.width > 800 ? 4 : (mainPane.width > 500 ? 2 : 1)
                    columnSpacing: 16
                    rowSpacing: 16

                    StatusCard {
                        icon: root.iconVideo
                        label: "Capture"
                        value: backend.captureAvailable ? (backend.recording ? "Live" : "Paused") : "Unavailable"
                        detail: backend.captureAvailable ? (backend.currentGame || "Desktop") : "Retry from settings"
                        accentColor: backend.captureAvailable ? root.accentBlue : root.recordRed
                    }

                    StatusCard {
                        icon: root.iconBuffer
                        label: "Buffer"
                        value: bufferReady() ? backend.bufferSeconds + "s ready" : "Warming up"
                        detail: bufferReady() ? backend.bufferPackets + " packets" : "Waiting"
                        accentColor: bufferReady() ? root.accent : root.warningYellow
                    }

                    StatusCard {
                        icon: root.iconOutput
                        label: "Video"
                        value: videoProfile()
                        detail: backend.captureCodec.toUpperCase() + " CQP " + backend.captureCqp
                        accentColor: root.accentPurple
                    }

                    StatusCard {
                        icon: root.iconAudio
                        label: "Audio"
                        value: backend.audioActive ? "Enabled" : "Disabled"
                        detail: backend.audioActive ? backend.audioSampleRate + " Hz" : "Loopback off"
                        accentColor: backend.audioActive ? root.warningYellow : root.inactive
                    }
                }

                GlassPanel {
                    id: recentSurface
                    theme: root.uiTheme
                    tone: "surface"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sheenOpacity: 0.9
                    depth: 0.95
                    color: Qt.rgba(0.038, 0.055, 0.058, 0.94)
                    border.color: Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.74)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 16

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: "Recent saves"
                                    color: root.textPrimary
                                    font: root.headingFont
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: totalClipCount() > overviewClips.length
                                        ? "Latest " + overviewClips.length + " of " + totalClipCount() + " saved clips"
                                        : overviewClips.length + (overviewClips.length === 1 ? " saved clip" : " saved clips") + " ready to share"
                                    color: root.textSoft
                                    font: root.bodyFont
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Item { Layout.fillWidth: true }

                            TextButton {
                                text: "All clips"
                                iconPath: root.iconGrid
                                Layout.preferredWidth: 110
                                onClicked: root.goToView(1, true)
                            }
                        }

                        ListView {
                            id: recentList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 10
                            model: overviewClips
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: CompactClipRow {
                                width: Math.max(0, recentList.width - 16)
                                clipData: modelData
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: recentList.count === 0
                                text: "No clips saved yet.\nPress " + root.shortcutText() + " when something cool happens."
                                color: root.textSoft
                                font: root.titleFont
                                width: Math.min(parent.width - 48, 420)
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                opacity: 0.6
                            }

                            ScrollBar.vertical: AppScrollBar {
                                theme: root.uiTheme
                                policy: recentList.contentHeight > recentList.height + 1 ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                                x: recentList.width - width
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            theme: root.uiTheme
            tone: "rail"
            Layout.preferredWidth: page.width >= 1120 ? 320 : 0
            Layout.minimumWidth: page.width >= 1120 ? 320 : 0
            Layout.maximumWidth: page.width >= 1120 ? 320 : 0
            Layout.fillHeight: true
            visible: page.width >= 1120
            opacity: page.appearOpacity
            transform: Translate { x: page.appearOffset }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "Capture health"
                        color: root.textPrimary
                        font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 20, weight: Font.DemiBold })
                    }

                    Text {
                        text: backend.captureAvailable ? "Recording status and output readiness." : "Capture needs attention before new clips can be saved."
                        color: root.textSoft
                        font: root.bodyFont
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                InspectorSection {
                    title: "PIPELINE"
                    rows: [
                        { label: "Capture", value: backend.captureAvailable ? (backend.recording ? "DXGI active" : "Paused") : "Unavailable", accent: backend.captureAvailable ? (backend.recording ? root.successGreen : root.inactive) : root.recordRed },
                        { label: "Encode", value: backend.captureCodec.toUpperCase() + " preset " + backend.capturePreset, accent: root.successGreen },
                        { label: "Buffer", value: bufferReady() ? backend.bufferSeconds + "s rolling window" : "Warming up", accent: bufferReady() ? root.accent : root.warningYellow },
                        { label: "Save", value: "MP4 export on demand", accent: root.textSecondary }
                    ]
                }

                InspectorSection {
                    title: "SESSION"
                    rows: [
                        { label: "Target", value: backend.currentGame || "Desktop", accent: root.successGreen },
                        { label: "Audio", value: backend.audioActive ? "WASAPI loopback" : "Disabled", accent: backend.audioActive ? root.successGreen : root.inactive },
                        { label: "Hotkey", value: root.shortcutText(), accent: root.accent },
                        { label: "Output", value: "Clips", accent: root.textSecondary }
                    ]
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: "WORKFLOW"
                        color: root.textMuted
                        font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 11, weight: Font.DemiBold, capitalization: Font.AllUppercase })
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

                    Rectangle { width: 4; height: 4; radius: 2; color: root.textSoft; opacity: 0.5 }

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
        appearOpacity = 0
        appearOffset = 25
        appearOpacity = 1.0
        appearOffset = 0
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
            spacing: 16

            GlassPanel {
                theme: root.uiTheme
                tone: "raised"
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                radius: 20
                sheenOpacity: 1.12
                depth: 1.18
                color: Qt.rgba(0.055, 0.095, 0.115, 0.92)
                border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.38)

                MonoIcon {
                    anchors.centerIn: parent
                    source: root.iconOverview
                    width: 32
                    height: 32
                    tint: root.accent
                    glowColor: root.accent
                    glowStrength: 0.3
                    treatment: "featured"
                    iconOpacity: 0.98
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "Overview"
                    color: root.textPrimary
                    font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 28, weight: Font.Bold })
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: backend.captureAvailable
                        ? (backend.recording ? "Capture is running. Save the last moment when something worth keeping happens." : "Capture is paused. Resume to rebuild the replay buffer.")
                        : "Capture needs attention. Existing clips, logs, and diagnostics are still available."
                    color: root.textSoft
                    font: root.bodyFont
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

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

    component StatusCard: GlassPanel {
        id: card

        property string icon
        property string label
        property string value
        property string detail
        property color accentColor: root.accent

        theme: root.uiTheme
        tone: "raised"
        Layout.fillWidth: true
        Layout.preferredHeight: 110
        sheenOpacity: mouse.containsMouse ? 1.2 : 0.8
        depth: mouse.containsMouse ? 1.1 : 0.9
        
        color: mouse.pressed ? root.pressedBg : mouse.containsMouse ? Qt.rgba(0.08, 0.12, 0.16, 0.94) : Qt.rgba(0.05, 0.07, 0.09, 0.8)
        border.color: mouse.containsMouse ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.4) : Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.5)

        Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

        Rectangle {
            x: 14
            y: 14
            width: 36
            height: 36
            radius: 18
            color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.1)
            
            MonoIcon {
                anchors.centerIn: parent
                source: card.icon
                width: 18
                height: 18
                tint: card.accentColor
                glowColor: card.accentColor
                glowStrength: 0.4
                treatment: "featured"
            }
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            spacing: 2

            Text {
                text: card.label
                color: root.textSoft
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 11, weight: Font.DemiBold, capitalization: Font.AllUppercase })
            }
            Text {
                text: card.value
                color: root.textPrimary
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 20, weight: Font.Bold })
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: card.detail
                color: root.textMuted
                font: root.smallFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
        }
    }

    component CompactClipRow: GlassPanel {
        id: clipRow

        property var clipData

        theme: root.uiTheme
        tone: "surface"
        height: 64
        sheenOpacity: rowMouse.containsMouse ? 1.0 : 0.70
        depth: rowMouse.containsMouse ? 1.04 : 0.82
        color: rowMouse.pressed ? root.pressedBg : rowMouse.containsMouse ? Qt.rgba(0.075, 0.115, 0.125, 0.92) : Qt.rgba(0.025, 0.038, 0.042, 0.78)
        border.color: rowMouse.containsMouse ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.4) : Qt.rgba(root.uiTheme.border.r, root.uiTheme.border.g, root.uiTheme.border.b, 0.64)

        Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 16

            AppThumbnail {
                theme: root.uiTheme
                Layout.preferredWidth: 84
                Layout.preferredHeight: 46
                clipPath: clipRow.clipData ? clipRow.clipData.path : ""
                fallbackText: "No preview"
                radius: themeTokens.radiusSm
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: clipRow.clipData ? clipRow.clipData.name : ""
                    color: root.textPrimary
                    font: Qt.font({ family: root.uiTheme.bodyFamily, pixelSize: 14, weight: Font.Medium })
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
                Layout.preferredWidth: 100
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
        implicitHeight: 36
    }

    component MiniIconButton: GlassPanel {
        id: miniIconButton

        property string icon
        property string tooltip
        property color accentColor: root.controlAccent(icon, false)
        signal clicked()

        theme: root.uiTheme
        tone: "raised"
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        color: miniMouse.pressed ? root.pressedBg : miniMouse.containsMouse ? Qt.rgba(0.12, 0.18, 0.22, 0.9) : root.panelRaised
        border.color: miniMouse.containsMouse ? accentColor : root.border

        Behavior on color { ColorAnimation { duration: 150 } }

        MonoIcon {
            anchors.centerIn: parent
            source: miniIconButton.icon
            width: 16
            height: 16
            tint: miniMouse.containsMouse ? miniIconButton.accentColor : root.textSecondary
            glowColor: miniMouse.containsMouse ? miniIconButton.accentColor : "transparent"
            glowStrength: miniMouse.containsMouse ? 0.3 : 0.0
            treatment: "compact"
            iconOpacity: miniMouse.containsMouse ? 1.0 : 0.78
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
            spacing: 12

            Text {
                text: inspectorSection.title
                color: root.textMuted
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 11, weight: Font.DemiBold, capitalization: Font.AllUppercase })
            }

            Repeater {
                model: inspectorSection.rows
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: modelData.accent
                    }

                    Text {
                        text: modelData.label
                        color: root.textSoft
                        font: root.bodyFont
                        Layout.preferredWidth: 64
                    }

                    Text {
                        text: modelData.value
                        color: root.textPrimary
                        font: root.bodyFont
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
        spacing: 14

        GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            radius: 13
            border.color: root.borderStrong
            color: Qt.rgba(0.1, 0.15, 0.2, 0.5)

            Text {
                anchors.centerIn: parent
                text: parent.parent.indexText
                color: root.textPrimary
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 12, weight: Font.Bold })
            }
        }

        Text {
            text: parent.detail
            color: root.textSecondary
            font: root.bodyFont
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            lineHeight: 1.2
        }
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: root.border
        opacity: 0.6
    }
}
