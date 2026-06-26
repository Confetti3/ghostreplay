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
        spacing: 14

        Item {
            id: mainPane
            Layout.fillWidth: true
            Layout.fillHeight: true
            opacity: page.appearOpacity
            transform: Translate { y: page.appearOffset }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 18

                OverviewHeader {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                }

                GlassPanel {
                    visible: !backend.captureAvailable
                    theme: root.uiTheme
                    tone: "raised"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    border.color: Qt.rgba(1.0, 0.52, 0.52, 0.40)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 46
                            radius: 0
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
                    targetDurationSec: backend.replayDurationSec
                    targetName: backend.currentGame || "Desktop"
                    shortcut: root.shortcutText()
                    exportBusy: backend.exportBusy
                    exportProgress: backend.exportProgress
                    exportStatus: backend.exportStatus
                    onSaveRequested: backend.saveClip()
                    onRetryRequested: backend.retryCapture()
                    Layout.preferredHeight: 108
                }

                GridLayout {
                    visible: backend.captureAvailable
                    Layout.fillWidth: true
                    columns: mainPane.width > 700 ? 3 : (mainPane.width > 500 ? 2 : 1)
                    columnSpacing: 14
                    rowSpacing: 14

                    StatusCard {
                        icon: root.iconVideo
                        label: "Capture"
                        value: backend.captureAvailable ? (backend.recording ? "Live" : "Paused") : "Unavailable"
                        detail: backend.captureAvailable ? (backend.currentGame || "Desktop") : "Retry from settings"
                        accentColor: backend.captureAvailable ? (backend.recording ? root.accent : root.inactive) : root.recordRed
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
                    sheenOpacity: 0
                    depth: 0
                    border.color: root.uiTheme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
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
                            spacing: 8
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
            Layout.preferredWidth: page.width >= 1180 ? 264 : 0
            Layout.minimumWidth: page.width >= 1180 ? 264 : 0
            Layout.maximumWidth: page.width >= 1180 ? 264 : 0
            Layout.fillHeight: true
            visible: page.width >= 1180
            opacity: page.appearOpacity
            transform: Translate { x: page.appearOffset }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 18

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "Output"
                        color: root.textPrimary
                        font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 20, weight: Font.DemiBold })
                    }

                    Text {
                        text: "Where clips land and how to save them."
                        color: root.textSoft
                        font: root.bodyFont
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                InspectorSection {
                    title: "OUTPUT"
                    rows: [
                        { label: "Target", value: backend.currentGame || "Desktop", accent: root.accent },
                        { label: "Hotkey", value: root.shortcutText(), accent: root.accent },
                        { label: "Folder", value: backend.outputDir, accent: root.textSecondary }
                    ]
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

                    Rectangle { width: 3; height: 3; radius: 0; color: root.textSoft; opacity: 0.5 }

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
            : (backend.captureDisplayResolution.length > 0 ? backend.captureDisplayResolution : "Native")
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
            spacing: 12

            GlassPanel {
                theme: root.uiTheme
                tone: "raised"
                Layout.preferredWidth: 52
                Layout.preferredHeight: 52
                radius: 0
                sheenOpacity: 0
                depth: 0
                border.color: root.uiTheme.border

                MonoIcon {
                    anchors.centerIn: parent
                    source: root.iconOverview
                    width: 26
                    height: 26
                    tint: root.textPrimary
                    glowColor: root.textPrimary
                    glowStrength: 0.22
                    treatment: "featured"
                    iconOpacity: 0.98
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 4

                Text {
                    text: "Overview"
                    color: root.textPrimary
                    font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 25, weight: Font.Bold })
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
                    accentColor: backend.captureAvailable ? (backend.recording ? root.accent : root.inactive) : root.recordRed
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
        Layout.preferredHeight: 92
        sheenOpacity: 0
        depth: 0
        border.color: cardMouse.containsMouse ? root.uiTheme.borderStrong : root.uiTheme.border

        MouseArea {
            id: cardMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
        }

        Rectangle {
            x: 12
            y: 12
            width: 32
            height: 32
            radius: 0
            color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.08)
            border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.28)
            border.width: 1

            MonoIcon {
                anchors.centerIn: parent
                source: card.icon
                width: 16
                height: 16
                tint: card.accentColor === root.accent ? root.textPrimary : Qt.lighter(card.accentColor, 1.35)
                glowColor: card.accentColor === root.accent ? root.textPrimary : card.accentColor
                glowStrength: 0.12
                treatment: "plain"
            }
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.leftMargin: 52
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 14
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            spacing: 2

            Text {
                text: card.label
                color: root.textSoft
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 11, weight: Font.DemiBold, capitalization: Font.AllUppercase })
            }
            Text {
                text: card.value
                color: root.textPrimary
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 18, weight: Font.Bold })
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

    }

    component CompactClipRow: GlassPanel {
        id: clipRow

        property var clipData

        theme: root.uiTheme
        tone: "surface"
        height: 58
        sheenOpacity: 0
        depth: 0
        color: rowMouse.pressed ? root.pressedBg : rowMouse.containsMouse ? root.hoverBg : Qt.rgba(root.uiTheme.surfaceAlt.r, root.uiTheme.surfaceAlt.g, root.uiTheme.surfaceAlt.b, 0.42)
        border.color: rowMouse.containsMouse ? root.uiTheme.borderStrong : root.uiTheme.border

        Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 12

            AppThumbnail {
                theme: root.uiTheme
                Layout.preferredWidth: 76
                Layout.preferredHeight: 42
                clipPath: clipRow.clipData ? clipRow.clipData.path : ""
                fallbackText: "No preview"
                radius: themeTokens.radiusSm
                hoverActive: rowMouse.containsMouse
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
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
                text: "Export"
                iconPath: root.iconExport
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
        Layout.preferredWidth: 34
        Layout.preferredHeight: 34
        color: miniMouse.pressed ? root.pressedBg : miniMouse.containsMouse ? root.hoverBg : "transparent"
        border.color: miniMouse.containsMouse ? root.uiTheme.borderStrong : root.uiTheme.border

        Behavior on color { ColorAnimation { duration: 150 } }

        MonoIcon {
            anchors.centerIn: parent
            source: miniIconButton.icon
            width: 16
            height: 16
            tint: miniMouse.containsMouse ? miniIconButton.accentColor : root.textSecondary
            glowColor: "transparent"
            glowStrength: 0.0
            treatment: "plain"
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
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        radius: 0
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
            radius: 0
            sheenOpacity: 0
            depth: 0

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
