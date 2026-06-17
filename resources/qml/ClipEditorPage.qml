import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Page {
    id: page
    background: Rectangle { color: "transparent" }

    property var sourceClip: null
    property real trimStartSec: 0
    property real trimEndSec: 1
    property real previewPositionSec: 0
    property int targetWidth: 1280
    property int targetHeight: 720
    property int targetFps: 60
    property int quality: 70
    property bool fitToSize: true
    property real targetSizeMb: 8
    property int audioBitrateKbps: 128
    property string outputName: ""
    property string exportMode: "compressed"
    property string activeExportSourcePath: ""
    property string exportErrorText: ""
    property bool advancedExport: false
    property real previewVolume: 1.0
    readonly property bool compactLayout: width < 860
    readonly property real sourceAspectRatio: sourceClip && sourceClip.width > 0 && sourceClip.height > 0
        ? sourceClip.width / Math.max(1, sourceClip.height)
        : 16 / 9
    readonly property real mediaDurationSec: mediaPlayer.duration > 0
        ? mediaPlayer.duration / 1000
        : (sourceClip && sourceClip.durationSec ? sourceClip.durationSec : 0)
    readonly property real effectiveDuration: Math.max(0.1, mediaDurationSec || 0.1)
    readonly property bool mediaLoaded: mediaPlayer.mediaStatus === MediaPlayer.LoadedMedia
        || mediaPlayer.mediaStatus === MediaPlayer.BufferedMedia
        || mediaPlayer.mediaStatus === MediaPlayer.BufferingMedia
        || mediaPlayer.mediaStatus === MediaPlayer.StalledMedia
        || mediaPlayer.mediaStatus === MediaPlayer.EndOfMedia
        || mediaPlayer.duration > 0
    readonly property bool videoPlaybackUsable: !!sourceClip && mediaLoaded && !mediaFailed
    readonly property bool videoPlaying: mediaPlayer.playbackState === MediaPlayer.PlayingState
    readonly property bool mediaLoading: !!sourceClip
        && !mediaLoaded
        && !mediaFailed
        && (mediaPlayer.mediaStatus === MediaPlayer.LoadingMedia
            || mediaPlayer.mediaStatus === MediaPlayer.BufferingMedia
            || clipPreview.busy)
    readonly property bool activeExportMatchesClip: !!sourceClip
        && activeExportSourcePath.length > 0
        && activeExportSourcePath === sourceClip.path
    readonly property bool exportSucceeded: activeExportMatchesClip
        && !backend.exportBusy
        && backend.exportOutputPath.length > 0
    readonly property bool exportFailed: activeExportMatchesClip
        && !backend.exportBusy
        && exportErrorText.length > 0
        && backend.exportOutputPath.length === 0
    readonly property bool exportCancelled: activeExportMatchesClip
        && !backend.exportBusy
        && backend.exportStatus === "Export cancelled."
        && backend.exportOutputPath.length === 0
        && exportErrorText.length === 0
    property bool mediaFailed: false
    property string mediaErrorText: ""
    property bool pendingInitialSeek: false

    signal doneRequested()

    function formatTime(seconds) {
        seconds = Math.max(0, seconds || 0)
        let whole = Math.round(seconds)
        let h = Math.floor(whole / 3600)
        let m = Math.floor((whole % 3600) / 60)
        let s = whole % 60
        if (h > 0)
            return h + ":" + String(m).padStart(2, "0") + ":" + String(s).padStart(2, "0")
        return m + ":" + String(s).padStart(2, "0")
    }

    function nearestFps(value) {
        const choices = [24, 30, 48, 60]
        if (!value || value <= 0)
            return 60
        let best = choices[0]
        let delta = Math.abs(value - best)
        for (let i = 1; i < choices.length; ++i) {
            let nextDelta = Math.abs(value - choices[i])
            if (nextDelta < delta) {
                best = choices[i]
                delta = nextDelta
            }
        }
        return best
    }

    function outputResolutionLabel() {
        return targetWidth > 0 && targetHeight > 0 ? targetWidth + "x" + targetHeight : "Source"
    }

    function clampTrimRange() {
        let duration = effectiveDuration
        trimStartSec = Math.max(0, Math.min(trimStartSec, Math.max(0, duration - 0.25)))
        trimEndSec = Math.max(trimStartSec + 0.25, Math.min(trimEndSec, duration))
        previewPositionSec = Math.max(trimStartSec, Math.min(previewPositionSec, trimEndSec))
    }

    function fileUrl(path) {
        if (!path)
            return ""
        return "file:///" + encodeURI(path.replace(/\\/g, "/")).replace(/#/g, "%23")
    }

    function setPreviewPosition(seconds, request) {
        clampTrimRange()
        previewPositionSec = Math.max(trimStartSec, Math.min(seconds || 0, trimEndSec))
        if (request && sourceClip) {
            if (videoPlaybackUsable)
                mediaPlayer.setPosition(Math.round(previewPositionSec * 1000))
            else
                clipPreview.requestFrame(previewPositionSec)
        }
    }

    function pausePreview() {
        if (mediaPlayer.playbackState !== MediaPlayer.StoppedState)
            mediaPlayer.pause()
    }

    function openForClip(nextClip) {
        mediaPlayer.stop()
        mediaPlayer.source = ""
        mediaFailed = false
        mediaErrorText = ""
        pendingInitialSeek = true
        sourceClip = nextClip
        trimStartSec = 0
        trimEndSec = Math.max(0.25, nextClip.durationSec || 0)
        previewPositionSec = trimStartSec
        targetWidth = nextClip.width > 0 ? nextClip.width : 1280
        targetHeight = nextClip.height > 0 ? nextClip.height : 720
        targetFps = nearestFps(nextClip.fps)
        quality = 70
        fitToSize = true
        targetSizeMb = 8
        audioBitrateKbps = 128
        previewVolume = 1.0
        activeExportSourcePath = ""
        exportErrorText = ""
        exportMode = "compressed"
        advancedExport = false
        let baseOutputName = (nextClip.name || "clip").replace(/\.mp4$/i, "")
        baseOutputName = baseOutputName.replace(/(_share)+$/i, "_share")
        outputName = /_share$/i.test(baseOutputName) ? baseOutputName : baseOutputName + "_share"
        clampTrimRange()
        clipPreview.loadClip(nextClip.path)
        mediaPlayer.source = fileUrl(nextClip.path)
        setPreviewPosition(trimStartSec, false)
    }

    function closeEditor() {
        mediaPlayer.stop()
        mediaPlayer.source = ""
        mediaFailed = false
        mediaErrorText = ""
        pendingInitialSeek = false
        clipPreview.clear()
        sourceClip = null
        doneRequested()
    }

    function exportModeLabel() {
        if (exportMode === "lossless_trim") return "Trim copy"
        if (exportMode === "original_quality") return "Export original"
        if (exportMode === "discord_25mb") return "Export Discord"
        if (exportMode === "twitter_512mb") return "Export Twitter/X"
        return "Export compressed"
    }

    function exportSummaryText() {
        if (exportMode === "lossless_trim") return "No re-encode. Uses source streams."
        if (exportMode === "original_quality") return "Hardware encode near source quality."
        if (exportMode === "discord_25mb") return "Targets 25 MB with retry passes."
        if (exportMode === "twitter_512mb") return "Targets 512 MB with retry passes."
        return "Hardware encode tuned for smaller sharing files."
    }

    function exportStatusTitle() {
        if (backend.exportBusy) return "Exporting share clip"
        if (exportSucceeded) return "Share clip is ready"
        if (exportFailed) return "Export needs attention"
        if (exportCancelled) return "Export cancelled"
        return "Ready to export"
    }

    function exportStatusDetail() {
        if (exportFailed) return exportErrorText
        if (backend.exportStatus.length > 0) return backend.exportStatus
        if (exportSucceeded) return backend.exportOutputPath
        return exportSummaryText()
    }

    function exportStatusAccent() {
        if (exportFailed) return root.recordRed
        if (exportCancelled) return root.warningYellow
        if (backend.exportBusy || exportSucceeded) return root.successGreen
        return root.accent
    }

    function exportPrimaryLabel() {
        if (backend.exportBusy) return "Cancel export"
        if (exportFailed) return "Retry export"
        if (exportSucceeded || exportCancelled) return "Export again"
        return exportModeLabel()
    }

    Connections {
        target: backend
        function onExportError(msg) {
            if (page.activeExportMatchesClip)
                page.exportErrorText = msg
        }
        function onExportStateChanged() {
            if (backend.exportBusy)
                page.exportErrorText = ""
        }
    }

    AudioOutput {
        id: previewAudio
        volume: page.previewVolume
    }

    MediaPlayer {
        id: mediaPlayer
        audioOutput: previewAudio
        videoOutput: previewVideo

        onPositionChanged: {
            if (!sourceClip)
                return
            let seconds = mediaPlayer.position / 1000
            if (seconds >= trimEndSec && videoPlaying) {
                mediaPlayer.pause()
                mediaPlayer.setPosition(Math.round(trimEndSec * 1000))
                page.previewPositionSec = trimEndSec
                return
            }
            page.previewPositionSec = Math.max(trimStartSec, Math.min(seconds, trimEndSec))
        }

        onDurationChanged: {
            if (sourceClip && mediaPlayer.duration > 0) {
                trimEndSec = Math.max(0.25, Math.min(trimEndSec, mediaPlayer.duration / 1000))
                clampTrimRange()
            }
        }

        onMediaStatusChanged: {
            if (!sourceClip)
                return
            if (mediaPlayer.mediaStatus === MediaPlayer.InvalidMedia) {
                mediaFailed = true
                mediaErrorText = mediaPlayer.errorString.length > 0 ? mediaPlayer.errorString : "Video playback unavailable"
                clipPreview.requestFrame(previewPositionSec)
                return
            }
            if (pendingInitialSeek && mediaLoaded) {
                pendingInitialSeek = false
                mediaPlayer.pause()
                mediaPlayer.setPosition(Math.round(trimStartSec * 1000))
                previewPositionSec = trimStartSec
            }
        }

        onErrorOccurred: function(error, errorString) {
            if (error === MediaPlayer.NoError)
                return
            mediaFailed = true
            mediaErrorText = errorString && errorString.length > 0 ? errorString : "Video playback unavailable"
            if (sourceClip)
                clipPreview.requestFrame(previewPositionSec)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            EditorButton {
                iconSource: root.iconBack
                text: "Back"
                onClicked: page.closeEditor()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text { text: "Share clip"; color: root.textPrimary; font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 21, weight: Font.DemiBold }) }
                Text {
                    text: sourceClip ? sourceClip.name : "Choose a clip from the library."
                    color: root.textSoft
                    font: root.smallFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            StatusPill { text: sourceClip ? formatTime(trimEndSec - trimStartSec) : "No clip"; accentColor: root.accent }
            StatusPill { visible: backend.exportBusy; text: backend.exportStatus || "Exporting"; accentColor: root.successGreen }
        }

        FlowSteps {
            Layout.fillWidth: true
            activeStep: backend.exportBusy || exportSucceeded || exportFailed || exportCancelled ? 2 : 0
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: compactLayout ? 1 : 2
            columnSpacing: 12
            rowSpacing: 12

            GlassPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: compactLayout ? 430 : 0
                theme: root.uiTheme
                tone: "surface"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    GlassPanel {
                        id: previewFrame
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 250
                        Layout.preferredHeight: Math.max(300, Math.min(560, width / sourceAspectRatio))
                        theme: root.uiTheme
                        tone: "alt"
                        color: "#040608"
                        border.color: root.borderStrong
                        clip: true

                        Rectangle {
                            anchors.fill: parent
                            color: "#050607"
                            visible: true
                        }

                        VideoOutput {
                            id: previewVideo
                            anchors.fill: parent
                            anchors.margins: 1
                            fillMode: VideoOutput.PreserveAspectFit
                            visible: page.videoPlaybackUsable
                        }

                        Image {
                            anchors.fill: parent
                            anchors.margins: 1
                            fillMode: Image.PreserveAspectFit
                            source: clipPreview.frameUrl
                            visible: sourceClip
                                && (!page.videoPlaybackUsable || page.mediaFailed)
                                && clipPreview.hasFrame
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: 10
                            visible: !sourceClip
                                || ((!page.videoPlaybackUsable || page.mediaFailed) && !clipPreview.hasFrame)

                            BusyIndicator {
                                anchors.horizontalCenter: parent.horizontalCenter
                                running: page.mediaLoading
                                visible: page.mediaLoading
                            }

                            Text {
                                width: Math.min(previewFrame.width - 48, 420)
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                color: root.textSoft
                                font: root.bodyFont
                                text: page.mediaFailed && page.mediaErrorText.length > 0
                                    ? page.mediaErrorText
                                    : clipPreview.error.length > 0
                                    ? clipPreview.error
                                    : (sourceClip ? "Preview unavailable" : "No clip selected")
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        EditorButton {
                            iconSource: page.videoPlaying ? root.iconPause : root.iconPlay
                            text: page.videoPlaying ? "Pause" : "Play"
                            enabled: page.videoPlaybackUsable
                            onClicked: {
                                if (page.videoPlaying) {
                                    mediaPlayer.pause()
                                } else {
                                    if (previewPositionSec >= trimEndSec)
                                        setPreviewPosition(trimStartSec, true)
                                    mediaPlayer.play()
                                }
                            }
                        }

                        EditorButton {
                            iconSource: root.iconTrimIn
                            text: "Jump In"
                            enabled: !!sourceClip
                            onClicked: {
                                pausePreview()
                                setPreviewPosition(trimStartSec, true)
                            }
                        }

                        EditorButton {
                            iconSource: root.iconTrimOut
                            text: "Jump Out"
                            enabled: !!sourceClip
                            onClicked: {
                                pausePreview()
                                setPreviewPosition(trimEndSec, true)
                            }
                        }

                        Slider {
                            id: scrubber
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(effectiveDuration, 1)
                            value: previewPositionSec
                            enabled: !!sourceClip
                            onMoved: {
                                pausePreview()
                                setPreviewPosition(value, true)
                            }
                        }

                        Text {
                            text: formatTime(previewPositionSec) + " / " + formatTime(effectiveDuration)
                            color: root.textSoft
                            font: root.smallFont
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        RowLayout {
                            Layout.preferredWidth: 58
                            spacing: 6
                            MonoIcon { source: root.iconVolume; Layout.preferredWidth: 14; Layout.preferredHeight: 14; iconOpacity: 0.54 }
                            Text {
                                text: "Volume"
                                color: root.textSoft
                                font: root.smallFont
                            }
                        }

                        Slider {
                            id: volumeSlider
                            Layout.preferredWidth: 160
                            from: 0
                            to: 1
                            stepSize: 0.01
                            value: page.previewVolume
                            enabled: page.videoPlaybackUsable
                            onMoved: {
                                page.previewVolume = value
                            }
                        }

                        Text {
                            text: Math.round(page.previewVolume * 100) + "%"
                            color: root.textSecondary
                            font: root.smallFont
                            Layout.preferredWidth: 38
                        }

                        Text {
                            text: sourceClip && !sourceClip.hasAudio ? "No audio stream" : ""
                            color: root.textSoft
                            font: root.smallFont
                            Layout.fillWidth: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Trim"; color: root.textSecondary; font: root.bodyFont }
                            Item { Layout.fillWidth: true }
                            Text { text: formatTime(trimStartSec) + " - " + formatTime(trimEndSec); color: root.textPrimary; font: root.bodyFont }
                        }

                        RangeSlider {
                            id: trimSlider
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(effectiveDuration, 1)
                            first.value: trimStartSec
                            second.value: trimEndSec
                        }

                        Connections {
                            target: trimSlider.first
                            function onMoved() {
                                pausePreview()
                                trimStartSec = Math.max(0, Math.min(trimSlider.first.value, trimEndSec - 0.25))
                                setPreviewPosition(trimStartSec, true)
                            }
                        }

                        Connections {
                            target: trimSlider.second
                            function onMoved() {
                                pausePreview()
                                trimEndSec = Math.max(trimSlider.second.value, trimStartSec + 0.25)
                                setPreviewPosition(trimEndSec, true)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text { text: "Clip length"; color: root.textSoft; font: root.smallFont }
                            Text { text: formatTime(trimEndSec - trimStartSec); color: root.textPrimary; font: root.bodyFont }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: sourceClip ? (sourceClip.resolution || "") + (sourceClip.fps ? "  " + Math.round(sourceClip.fps) + " fps" : "") : ""
                                color: root.textSoft
                                font: root.smallFont
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: page.mediaFailed || !page.videoPlaybackUsable
                                ? (clipPreview.hasFrame ? "Still preview" : "Preview unavailable")
                                : (page.videoPlaying ? "Preview playing" : "Preview paused")
                            color: root.textMuted
                            font: root.smallFont
                        }
                    }
                }
            }

            GlassPanel {
                Layout.preferredWidth: compactLayout ? -1 : 380
                Layout.minimumWidth: compactLayout ? 0 : 340
                Layout.maximumWidth: compactLayout ? 16777215 : 400
                Layout.fillWidth: compactLayout
                Layout.fillHeight: true
                Layout.minimumHeight: compactLayout ? 420 : 0
                theme: root.uiTheme
                tone: "surface"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    Text { text: "Export"; color: root.textPrimary; font: root.titleFont }

                    ScrollView {
                        id: exportScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.vertical: AppScrollBar { theme: root.uiTheme }

                        ColumnLayout {
                            width: exportScroll.availableWidth
                            spacing: 12

                            Text {
                                Layout.fillWidth: true
                                text: "Build a lighter share clip without changing the original file."
                                color: root.textSoft
                                font: root.smallFont
                                wrapMode: Text.Wrap
                            }

                            FieldGroup {
                                label: "Preset"
                                ComboBox {
                                    id: presetBox
                                    Layout.fillWidth: true
                                    textRole: "label"
                                    valueRole: "mode"
                                    model: [
                                        { label: "Lossless trim copy", mode: "lossless_trim" },
                                        { label: "Original quality", mode: "original_quality" },
                                        { label: "Compressed", mode: "compressed" },
                                        { label: "Discord ready (<25 MB)", mode: "discord_25mb" },
                                        { label: "Twitter/X ready (<512 MB)", mode: "twitter_512mb" }
                                    ]
                                    currentIndex: {
                                        for (let i = 0; i < model.length; ++i) {
                                            if (model[i].mode === exportMode)
                                                return i
                                        }
                                        return 2
                                    }
                                    onActivated: exportMode = model[index].mode
                                }
                            }

                            FieldGroup {
                                label: "Output name"
                                TextField {
                                    Layout.fillWidth: true
                                    text: outputName
                                    color: root.textPrimary
                                    selectByMouse: true
                                    background: FieldBackground {}
                                    onTextChanged: outputName = text
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Switch {
                                    checked: advancedExport
                                    onToggled: advancedExport = checked
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text { text: "Advanced export controls"; color: root.textSecondary; font: root.bodyFont }
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Override resolution, frame rate, quality, and audio settings."
                                        color: root.textSoft
                                        font: root.smallFont
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }

                            FieldGroup {
                                visible: advancedExport && exportMode !== "lossless_trim"
                                label: "Resolution"
                                ComboBox {
                                    id: resolutionBox
                                    Layout.fillWidth: true
                                    model: [
                                        { label: "Source", width: sourceClip && sourceClip.width ? sourceClip.width : 1280, height: sourceClip && sourceClip.height ? sourceClip.height : 720 },
                                        { label: "1920 x 1080", width: 1920, height: 1080 },
                                        { label: "1600 x 900", width: 1600, height: 900 },
                                        { label: "1280 x 720", width: 1280, height: 720 },
                                        { label: "854 x 480", width: 854, height: 480 }
                                    ]
                                    textRole: "label"
                                    onActivated: {
                                        let choice = model[index]
                                        targetWidth = choice.width
                                        targetHeight = choice.height
                                    }
                                }
                            }

                            FieldGroup {
                                visible: advancedExport && exportMode !== "lossless_trim"
                                label: "Frame rate"
                                ComboBox {
                                    id: fpsBox
                                    Layout.fillWidth: true
                                    model: [24, 30, 48, 60]
                                    currentIndex: Math.max(0, model.indexOf(targetFps))
                                    onActivated: targetFps = model[index]
                                }
                            }

                            FieldGroup {
                                visible: advancedExport && exportMode !== "lossless_trim"
                                label: "Quality"
                                Slider {
                                    Layout.fillWidth: true
                                    from: 30
                                    to: 100
                                    stepSize: 5
                                    value: quality
                                    onMoved: quality = Math.round(value)
                                }
                                Text { text: quality + "%"; color: root.textPrimary; font: root.smallFont }
                            }

                            RowLayout {
                                visible: advancedExport && exportMode !== "lossless_trim"
                                Layout.fillWidth: true
                                spacing: 8
                                Switch {
                                    checked: fitToSize
                                    onToggled: fitToSize = checked
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text { text: "Fit to share size"; color: root.textSecondary; font: root.bodyFont }
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Auto-adjust bitrate to stay close to the target size."
                                        color: root.textSoft
                                        font: root.smallFont
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }

                            FieldGroup {
                                visible: advancedExport && fitToSize && exportMode !== "lossless_trim"
                                label: "Target size (MB)"
                                enabled: fitToSize
                                opacity: fitToSize ? 1 : 0.55
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 3
                                    to: 200
                                    value: targetSizeMb
                                    editable: true
                                    onValueModified: targetSizeMb = value
                                }
                            }

                            FieldGroup {
                                visible: advancedExport && exportMode !== "lossless_trim"
                                label: "Audio bitrate"
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: [96, 128, 160, 192]
                                    currentIndex: Math.max(0, model.indexOf(audioBitrateKbps))
                                    onActivated: audioBitrateKbps = model[index]
                                }
                            }

                            GlassPanel {
                                Layout.fillWidth: true
                                theme: root.uiTheme
                                tone: "raised"
                                implicitHeight: 94

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4
                                    Text { text: "Export summary"; color: root.textMuted; font: root.smallFont }
                                    Text { text: "Length: " + formatTime(trimEndSec - trimStartSec); color: root.textPrimary; font: root.bodyFont }
                                    Text { text: exportSummaryText(); color: root.textSecondary; font: root.smallFont }
                                    Text {
                                        text: exportMode === "lossless_trim"
                                            ? "Output: source streams"
                                            : "Output: " + outputResolutionLabel() + "  " + targetFps + " fps"
                                        color: root.textSoft
                                        font: root.smallFont
                                    }
                                }
                            }
                        }
                    }

                    ExportStateCard {
                        Layout.fillWidth: true
                        visible: backend.exportBusy || exportSucceeded || exportFailed || exportCancelled
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        EditorButton {
                            Layout.fillWidth: true
                            iconSource: backend.exportBusy ? root.iconPause : root.iconExport
                            text: exportPrimaryLabel()
                            primary: true
                            enabled: !!sourceClip
                            onClicked: {
                                if (backend.exportBusy) {
                                    backend.cancelExport()
                                } else {
                                    page.activeExportSourcePath = sourceClip.path
                                    page.exportErrorText = ""
                                    backend.exportClip({
                                        path: sourceClip.path,
                                        durationSec: sourceClip.durationSec,
                                        hasAudio: sourceClip.hasAudio,
                                        width: sourceClip.width,
                                        height: sourceClip.height,
                                        trimStartSec: trimStartSec,
                                        trimEndSec: trimEndSec,
                                        exportMode: exportMode,
                                        targetWidth: targetWidth,
                                        targetHeight: targetHeight,
                                        targetFps: targetFps,
                                        quality: quality,
                                        fitToSize: fitToSize,
                                        targetSizeMb: targetSizeMb,
                                        audioBitrateKbps: audioBitrateKbps,
                                        codec: backend.captureCodec,
                                        outputName: outputName
                                    })
                                }
                            }
                        }

                        EditorButton {
                            Layout.preferredWidth: 132
                            iconSource: root.iconBack
                            text: "Back to clips"
                            enabled: !backend.exportBusy
                            onClicked: page.closeEditor()
                        }
                    }
                }
            }
        }
    }

    component ExportStateCard: GlassPanel {
        id: exportStateCard
        theme: root.uiTheme
        tone: "raised"
        border.color: page.exportStatusAccent()
        color: Qt.rgba(0.05, 0.08, 0.08, 0.94)
        implicitHeight: contentColumn.implicitHeight + 24

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    radius: 5
                    color: page.exportStatusAccent()
                    opacity: backend.exportBusy ? 0.88 : 1.0

                    SequentialAnimation on opacity {
                        running: backend.exportBusy
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.38; duration: 760; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 0.96; duration: 760; easing.type: Easing.InOutQuad }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        Layout.fillWidth: true
                        text: page.exportStatusTitle()
                        color: root.textPrimary
                        font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 15, weight: Font.DemiBold })
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: page.exportStatusDetail()
                        color: page.exportFailed ? root.recordRed : root.textSoft
                        font: root.smallFont
                        wrapMode: Text.Wrap
                        maximumLineCount: page.exportFailed ? 4 : 2
                        elide: Text.ElideRight
                    }
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: page.exportSucceeded ? 1 : (backend.exportBusy ? backend.exportProgress : 0)
                visible: backend.exportBusy || page.exportSucceeded
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: page.exportSucceeded

                EditorButton {
                    Layout.fillWidth: true
                    iconSource: root.iconOpen
                    text: "Open clip"
                    onClicked: backend.openClip(backend.exportOutputPath)
                }

                EditorButton {
                    Layout.fillWidth: true
                    iconSource: root.iconReveal
                    text: "Show folder"
                    onClicked: backend.revealInExplorer(backend.exportOutputPath)
                }

                EditorButton {
                    Layout.fillWidth: true
                    iconSource: root.iconClipboard
                    text: "Copy"
                    onClicked: backend.copyClipToClipboard(backend.exportOutputPath)
                }
            }
        }
    }

    component FlowSteps: GlassPanel {
        id: flowSteps
        property int activeStep: 0
        theme: root.uiTheme
        tone: "raised"
        Layout.preferredHeight: 48

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 8

            StepPill { label: "Trim"; selected: flowSteps.activeStep === 0; accentColor: root.warningYellow }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.border }
            StepPill { label: "Preset"; selected: flowSteps.activeStep === 1; accentColor: root.accentPurple }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.border }
            StepPill { label: backend.exportBusy ? "Exporting" : "Export"; selected: flowSteps.activeStep === 2; accentColor: root.successGreen }
        }
    }

    component StepPill: GlassPanel {
        id: stepPill
        property string label
        property bool selected: false
        property color accentColor: root.accent
        theme: root.uiTheme
        tone: "surface"
        Layout.preferredWidth: Math.max(82, stepLabel.implicitWidth + 34)
        Layout.preferredHeight: 28
        radius: 14
        color: selected ? Qt.rgba(0.08, 0.13, 0.12, 0.96) : "transparent"
        border.color: selected ? accentColor : "transparent"

        RowLayout {
            anchors.centerIn: parent
            spacing: 7

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: stepPill.selected ? stepPill.accentColor : root.borderStrong
                opacity: stepPill.selected ? 1.0 : 0.72
            }

            Text {
                id: stepLabel
                text: stepPill.label
                color: stepPill.selected ? root.textPrimary : root.textSoft
                font: Qt.font({ family: root.uiTheme.displayFamily, pixelSize: 12, weight: stepPill.selected ? Font.DemiBold : Font.Medium })
            }
        }
    }

    component FieldBackground: GlassPanel {
        theme: root.uiTheme
        tone: "raised"
        color: root.panelRaised
    }

    component FieldGroup: ColumnLayout {
        property string label
        Layout.fillWidth: true
        spacing: 6
        Text { text: parent.label; color: root.textMuted; font: root.smallFont }
    }

    component StatusPill: GlassPanel {
        property string text
        property color accentColor: root.accent
        theme: root.uiTheme
        tone: "raised"
        radius: 13
        color: root.panelRaised
        implicitWidth: tagLabel.implicitWidth + 22
        implicitHeight: 26

        RowLayout {
            anchors.centerIn: parent
            spacing: 6
            Rectangle { width: 6; height: 6; radius: 3; color: parent.parent.accentColor }
            Text { id: tagLabel; text: parent.parent.text; color: root.textSecondary; font: root.smallFont }
        }
    }

    component EditorButton: Button {
        id: editorButton
        property string iconSource
        property bool primary: false
        font: root.bodyFont
        implicitHeight: 36
        implicitWidth: Math.max(80, editorContent.implicitWidth + leftPadding + rightPadding)
        leftPadding: 12
        rightPadding: 12
        topPadding: 0
        bottomPadding: 0
        background: GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            radius: 8
            color: !editorButton.enabled
                ? Qt.rgba(0.03, 0.05, 0.08, 0.72)
                : editorButton.primary
                    ? (editorButton.hovered ? Qt.rgba(0.16, 0.18, 0.10, 0.98) : Qt.rgba(0.13, 0.15, 0.09, 0.94))
                    : (editorButton.hovered ? root.hoverBg : root.panelRaised)
            border.color: editorButton.activeFocus
                ? root.accent
                : editorButton.primary
                    ? root.warningYellow
                    : (editorButton.enabled ? (editorButton.hovered ? root.glassLine : root.border) : root.border)
            border.width: editorButton.activeFocus ? 2 : 1
        }
        contentItem: RowLayout {
            id: editorContent
            spacing: editorButton.iconSource.length > 0 ? 7 : 0
            MonoIcon {
                visible: editorButton.iconSource.length > 0
                source: editorButton.iconSource
                Layout.preferredWidth: 15
                Layout.preferredHeight: 15
                tint: editorButton.primary ? root.warningYellow : (editorButton.hovered ? root.controlAccent(editorButton.iconSource, false) : root.textSecondary)
                glowColor: editorButton.primary ? root.warningYellow : "transparent"
                glowStrength: editorButton.primary ? 0.08 : 0.0
                iconOpacity: editorButton.enabled ? 0.82 : 0.32
            }
            Text {
                text: editorButton.text
                color: editorButton.enabled ? root.textPrimary : root.textSoft
                font: editorButton.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
        Accessible.role: Accessible.Button
        Accessible.name: editorButton.text
    }
}
