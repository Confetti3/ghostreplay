import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: page

    background: Rectangle { color: "transparent" }
    property int activeCategory: 0
    property bool pendingSettings: false
    property string draftCaptureSource: backend.captureSource
    property string draftEncoderBackend: backend.encoderBackend
    property int draftCaptureCqp: backend.captureCqp
    property int draftCapturePreset: backend.capturePreset
    property int draftCaptureWidth: backend.captureWidth
    property int draftCaptureHeight: backend.captureHeight
    property int draftCaptureFps: backend.captureFps
    property int draftKeyframeInterval: backend.keyframeInterval
    property bool draftAudioEnabled: backend.audioEnabled
    property int draftAudioChannels: backend.audioChannels
    property int draftAudioSampleRate: backend.audioSampleRate
    property int draftAudioBitrateKbps: backend.audioBitrateKbps
    property int draftReplayDurationSec: backend.replayDurationSec
    property int draftHotkeyModifiers: backend.hotkeyModifiers
    property int draftHotkeyVk: backend.hotkeyVk
    property string draftOutputDir: backend.outputDir
    property bool draftLaunchAtStartup: backend.launchAtStartup
    property int draftMonitorIndex: backend.monitorIndex
    property string saveMessage: "Settings are up to date."
    property bool saveError: false
    property string hotkeyStatus: "Click the recorder, then press a modifier with a key."
    property bool hotkeyStatusError: false

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.narrowShell ? 8 : 16
        anchors.rightMargin: root.narrowShell ? 8 : 16
        anchors.topMargin: root.narrowShell ? 8 : 16
        anchors.bottomMargin: root.narrowShell ? 64 : 72
        spacing: root.narrowShell ? 8 : 14

        GlassPanel {
            theme: root.uiTheme
            tone: "rail"
            Layout.preferredWidth: root.narrowShell ? 78 : 240
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: root.narrowShell ? 8 : 14
                spacing: 10

                ColumnLayout {
                    visible: !root.narrowShell
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "Settings"; color: root.textPrimary; font: root.headingFont }
                    Text { text: "Profile and output"; color: root.textSoft; font: root.smallFont; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }

                Divider { visible: !root.narrowShell }

                Repeater {
                    model: [
                        { icon: root.iconVideo, label: "Video", desc: "Codec, source, resolution" },
                        { icon: root.iconAudio, label: "Audio", desc: "Loopback and AAC profile" },
                        { icon: root.iconBuffer, label: "Buffer", desc: "Replay duration and memory" },
                        { icon: root.iconHotkey, label: "Hotkey", desc: "Save shortcut" },
                        { icon: root.iconOutput, label: "Output", desc: "Folder and startup" }
                    ]

                    CategoryRow {
                        icon: modelData.icon
                        title: modelData.label
                        subtitle: modelData.desc
                        selected: page.activeCategory === index
                        onClicked: page.activeCategory = index
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        GlassPanel {
            theme: root.uiTheme
            tone: "surface"
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                anchors.leftMargin: root.narrowShell ? 12 : 20
                anchors.rightMargin: root.narrowShell ? 12 : 20
                anchors.topMargin: root.narrowShell ? 12 : 20
                anchors.bottomMargin: root.narrowShell ? 12 : 20
                currentIndex: page.activeCategory

                SettingsPane {
                    title: "Video"
                    description: backend.captureAvailable ? "Capture source and encoding settings for replay recording." : "Recording is unavailable on this system. Settings remain editable for the next successful startup."

                    SettingRow {
                        title: "Capture source"
                        desc: "Window uses the Windows picker. Game captures the foreground game window without hooks."
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: root.narrowShell ? 160 : 220
                            model: ["Desktop", "Window", "Game"]
                            currentIndex: {
                                if (page.draftCaptureSource === "window") return 1
                                if (page.draftCaptureSource === "game") return 2
                                return 0
                            }
                            onActivated: {
                                let values = ["desktop", "window", "game"]
                                page.draftCaptureSource = values[currentIndex]
                                page.pendingSettings = true
                            }
                        }
                    }

                    SettingRow {
                        title: "Target display"
                        desc: "Select the display to capture when capturing the Desktop."
                        enabled: page.draftCaptureSource === "desktop"
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: root.narrowShell ? 160 : 220
                            model: {
                                let list = backend.availableDisplays
                                let names = []
                                for (let i = 0; i < list.length; i++) {
                                    names.push(list[i].label)
                                }
                                return names
                            }
                            currentIndex: {
                                let list = backend.availableDisplays
                                for (let i = 0; i < list.length; i++) {
                                    if (list[i].index === page.draftMonitorIndex)
                                        return i
                                }
                                return 0
                            }
                            onActivated: {
                                let list = backend.availableDisplays
                                if (currentIndex >= 0 && currentIndex < list.length) {
                                    page.draftMonitorIndex = list[currentIndex].index
                                    page.pendingSettings = true
                                }
                            }
                        }
                    }

                    SettingRow {
                        title: "Codec"
                        desc: "H.264 is the public beta capture codec. HEVC capture is disabled until replay-save muxing is validated."
                        StaticField {
                            Layout.preferredWidth: 150
                            text: "H.264"
                        }
                    }

                    SettingRow {
                        title: "Video encoder"
                        desc: "Auto chooses the best available encoder for the selected display."
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: root.narrowShell ? 160 : 220
                            model: ["Auto", "Nvidia NVENC", "AMD AMF", "Intel Quick Sync", "Windows compatibility"]
                            currentIndex: {
                                let value = page.draftEncoderBackend
                                if (value === "nvenc") return 1
                                if (value === "amf") return 2
                                if (value === "qsv") return 3
                                if (value === "mf") return 4
                                return 0
                            }
                            onActivated: {
                                let values = ["auto", "nvenc", "amf", "qsv", "mf"]
                                page.draftEncoderBackend = values[currentIndex]
                                page.pendingSettings = true
                            }
                        }
                    }

                    SettingRow {
                        title: "Quality"
                        desc: "Lower CQP means higher quality and larger files."
                        restartsCapture: true
                        RowLayout {
                            spacing: 6
                            NumberField { value: page.draftCaptureCqp; from: 0; to: 51; onCommit: function(nextValue) { page.draftCaptureCqp = nextValue; page.pendingSettings = true } }
                            UnitText { text: "CQP" }
                        }
                    }

                    SettingRow {
                        title: "Preset"
                        desc: "Lower presets favor quality. Higher presets favor speed."
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: root.narrowShell ? 140 : 180
                            model: ["1 - Slowest", "2", "3", "4 - Balanced", "5", "6", "7 - Fastest"]
                            currentIndex: page.draftCapturePreset - 1
                            onActivated: { page.draftCapturePreset = currentIndex + 1; page.pendingSettings = true }
                        }
                    }

                    SettingRow {
                        title: "Resolution"
                        desc: "Auto native uses 0 x 0 and adapts to display resolution and DPI changes."
                        restartsCapture: true
                        RowLayout {
                            spacing: 6
                            NumberField { value: page.draftCaptureWidth; from: 0; to: 7680; onCommit: function(nextValue) { page.draftCaptureWidth = nextValue; page.pendingSettings = true } }
                            Text { text: "x"; color: root.textSoft; font: root.bodyFont }
                            NumberField { value: page.draftCaptureHeight; from: 0; to: 4320; onCommit: function(nextValue) { page.draftCaptureHeight = nextValue; page.pendingSettings = true } }
                        }
                    }

                    SettingRow {
                        title: "Display status"
                        desc: backend.autoDisplayAdaptationEnabled
                            ? "Auto native is active; capture follows resolution and DPI changes."
                            : "Manual resolution is active; display changes keep your chosen output size."
                        StaticField {
                            Layout.fillWidth: true
                            text: backend.captureDisplayLabel + "  " + backend.captureDisplayResolution + "  " + backend.captureDisplayScale
                            font: root.smallFont
                            accentColor: backend.autoDisplayAdaptationEnabled ? root.accentCyan : root.border
                        }
                    }

                    SettingRow {
                        title: "Frame rate"
                        desc: "Target FPS for the encoded replay stream."
                        restartsCapture: true
                        RowLayout {
                            spacing: 6
                            NumberField { value: page.draftCaptureFps; from: 1; to: 240; onCommit: function(nextValue) { page.draftCaptureFps = nextValue; page.pendingSettings = true } }
                            UnitText { text: "FPS" }
                        }
                    }

                    SettingRow {
                        title: "Keyframe interval"
                        desc: "Spacing between keyframes. Lower seeks faster but grows output."
                        restartsCapture: true
                        RowLayout {
                            spacing: 6
                            NumberField { value: page.draftKeyframeInterval; from: 1; to: 600; onCommit: function(nextValue) { page.draftKeyframeInterval = nextValue; page.pendingSettings = true } }
                            UnitText { text: "frames" }
                        }
                    }
                }

                SettingsPane {
                    title: "Audio"
                    description: "System audio loopback capture and AAC settings."

                    SettingRow {
                        title: "Audio capture"
                        desc: "Enable WASAPI loopback capture for system output."
                        restartsCapture: true
                        AppToggle {
                            theme: root.uiTheme
                            checked: page.draftAudioEnabled
                            text: "Audio capture"
                            onToggled: { page.draftAudioEnabled = checked; page.pendingSettings = true }
                        }
                    }

                    SettingRow {
                        title: "Channels"
                        desc: "Stereo is the default capture layout."
                        enabled: page.draftAudioEnabled
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: 150
                            enabled: page.draftAudioEnabled
                            model: ["Mono", "Stereo"]
                            currentIndex: page.draftAudioChannels === 1 ? 0 : 1
                            onActivated: { page.draftAudioChannels = currentIndex === 0 ? 1 : 2; page.pendingSettings = true }
                        }
                    }

                    SettingRow {
                        title: "Sample rate"
                        desc: "48 kHz is a good default for game and desktop audio."
                        enabled: page.draftAudioEnabled
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: 160
                            enabled: page.draftAudioEnabled
                            model: ["44100 Hz", "48000 Hz", "96000 Hz"]
                            currentIndex: page.draftAudioSampleRate === 44100 ? 0 : (page.draftAudioSampleRate === 96000 ? 2 : 1)
                            onActivated: {
                                if (currentIndex === 0) page.draftAudioSampleRate = 44100
                                else if (currentIndex === 1) page.draftAudioSampleRate = 48000
                                else page.draftAudioSampleRate = 96000
                                page.pendingSettings = true
                            }
                        }
                    }

                    SettingRow {
                        title: "Bitrate"
                        desc: "AAC bitrate for exported clips."
                        enabled: page.draftAudioEnabled
                        restartsCapture: true
                        FlatCombo {
                            Layout.preferredWidth: 160
                            enabled: page.draftAudioEnabled
                            model: ["64 kbps", "128 kbps", "192 kbps", "256 kbps", "320 kbps"]
                            currentIndex: {
                                let b = page.draftAudioBitrateKbps
                                if (b <= 64) return 0
                                if (b <= 128) return 1
                                if (b <= 192) return 2
                                if (b <= 256) return 3
                                return 4
                            }
                            onActivated: {
                                let vals = [64, 128, 192, 256, 320]
                                page.draftAudioBitrateKbps = vals[currentIndex]
                                page.pendingSettings = true
                            }
                        }
                    }
                }

                SettingsPane {
                    title: "Buffer"
                    description: "How much recent footage stays in memory before it rolls out."

                    SettingRow {
                        title: "Replay duration"
                        desc: "Longer windows keep more history at the cost of RAM."
                        restartsCapture: true
                        RowLayout {
                            spacing: 6
                        NumberField { value: page.draftReplayDurationSec; from: 15; to: 3600; onCommit: function(nextValue) { page.draftReplayDurationSec = nextValue; page.pendingSettings = true } }
                        UnitText { text: "seconds" }
                        }
                    }
                }

                SettingsPane {
                    title: "Hotkey"
                    description: "Save the current replay buffer with a keyboard shortcut."

                    SettingRow {
                        title: "Save replay shortcut"
                        desc: "Record a modifier plus F-key, letter, or number. Save applies the new shortcut."
                        RowLayout {
                            spacing: 8
                            ShortcutRecorder {
                                modifiers: page.draftHotkeyModifiers
                                vk: page.draftHotkeyVk
                                onRecorded: function(nextModifiers, nextVk) {
                                    page.draftHotkeyModifiers = nextModifiers
                                    page.draftHotkeyVk = nextVk
                                    page.hotkeyStatus = "Shortcut staged: " + page.shortcutLabel(nextModifiers, nextVk) + "."
                                    page.hotkeyStatusError = false
                                    page.pendingSettings = true
                                }
                            }
                            ActionFieldButton {
                                iconPath: root.iconRefresh
                                text: "Reset"
                                onClicked: {
                                    page.resetHotkey()
                                    page.pendingSettings = true
                                }
                            }
                        }
                    }

                    SettingRow {
                        title: "Shortcut status"
                        desc: page.hotkeyStatus
                        StaticField {
                            Layout.fillWidth: true
                            text: page.shortcutLabel(page.draftHotkeyModifiers, page.draftHotkeyVk)
                            font: root.monoFont
                            accentColor: page.hotkeyStatusError ? root.recordRed : root.accent
                        }
                    }
                }

                SettingsPane {
                    title: "Output"
                    description: "Choose where clips land and whether the app starts with Windows."

                    SettingRow {
                        title: "Recording support"
                        desc: backend.captureAvailable ? "Startup checks passed for this session." : (backend.captureError || "Recording requires Windows desktop capture and a supported encoder.")
                        RowLayout {
                            spacing: 8
                            ActionFieldButton { iconPath: root.iconRefresh; text: "Retry"; onClicked: backend.retryCapture() }
                            ActionFieldButton { iconPath: root.iconOutput; text: "Diagnostics"; onClicked: backend.copyDiagnostics() }
                            ActionFieldButton { iconPath: root.iconOpen; text: "Log"; onClicked: backend.openLogFile() }
                        }
                    }

                    SettingRow {
                        title: "Save location"
                        desc: "Directory used for exported clip files."
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            StaticField {
                                Layout.fillWidth: true
                                text: page.draftOutputDir
                                font: root.smallFont
                            }

                            ActionFieldButton { iconPath: root.iconFolder; text: "Browse"; onClicked: folderDialog.open() }
                        }
                    }

                    SettingRow {
                        title: "Launch at startup"
                        desc: "Start Ghost Replay when Windows signs you in."
                        AppToggle {
                            theme: root.uiTheme
                            text: "Launch at startup"
                            checked: page.draftLaunchAtStartup
                            onToggled: { page.draftLaunchAtStartup = checked; page.pendingSettings = true }
                        }
                    }
                }
            }
        }
    }

    GlassPanel {
        theme: root.uiTheme
        tone: "chrome"
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 56
        radius: 0
        border.width: 0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10

            Text {
                text: saveToast.running
                    ? page.saveMessage
                    : (page.pendingSettings ? "Unsaved changes. Save applies them; capture changes may briefly restart recording." : page.saveMessage)
                color: page.saveError ? root.recordRed : (saveToast.running ? root.successGreen : root.textSoft)
                font: root.smallFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            AppButton {
                theme: root.uiTheme
                iconSource: root.iconRefresh
                text: "Revert"
                enabled: page.pendingSettings
                onClicked: {
                    page.syncDraftsFromBackend()
                    page.pendingSettings = false
                    page.saveError = false
                    page.saveMessage = "Settings are up to date."
                }
            }

            SaveButton {
                enabled: page.pendingSettings
                onClicked: {
                    page.applyDrafts()
                    if (backend.saveConfig()) {
                        page.pendingSettings = false
                        page.saveError = false
                        page.saveMessage = "Settings saved."
                        if (page.activeCategory === 3) {
                            page.hotkeyStatus = "Shortcut saved: " + page.shortcutLabel(page.draftHotkeyModifiers, page.draftHotkeyVk) + "."
                            page.hotkeyStatusError = false
                        }
                    } else {
                        page.saveError = true
                        page.saveMessage = "Settings could not be saved. Check file permissions and try again."
                    }
                    saveToast.restart()
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        currentFolder: "file:///" + page.draftOutputDir
        onAccepted: {
            page.draftOutputDir = selectedFolder.toString().replace("file:///", "")
            page.pendingSettings = true
        }
    }

    Timer {
        id: saveToast
        interval: 2200
        repeat: false
    }

    function applyDrafts() {
        backend.captureSource = draftCaptureSource
        backend.encoderBackend = draftEncoderBackend
        backend.captureCqp = draftCaptureCqp
        backend.capturePreset = draftCapturePreset
        backend.captureWidth = draftCaptureWidth
        backend.captureHeight = draftCaptureHeight
        backend.captureFps = draftCaptureFps
        backend.keyframeInterval = draftKeyframeInterval
        backend.audioEnabled = draftAudioEnabled
        backend.audioChannels = draftAudioChannels
        backend.audioSampleRate = draftAudioSampleRate
        backend.audioBitrateKbps = draftAudioBitrateKbps
        backend.replayDurationSec = draftReplayDurationSec
        backend.hotkeyModifiers = draftHotkeyModifiers
        backend.hotkeyVk = draftHotkeyVk
        backend.outputDir = draftOutputDir
        backend.launchAtStartup = draftLaunchAtStartup
        backend.monitorIndex = draftMonitorIndex
    }

    function syncDraftsFromBackend() {
        draftCaptureSource = backend.captureSource
        draftEncoderBackend = backend.encoderBackend
        draftCaptureCqp = backend.captureCqp
        draftCapturePreset = backend.capturePreset
        draftCaptureWidth = backend.captureWidth
        draftCaptureHeight = backend.captureHeight
        draftCaptureFps = backend.captureFps
        draftKeyframeInterval = backend.keyframeInterval
        draftAudioEnabled = backend.audioEnabled
        draftAudioChannels = backend.audioChannels
        draftAudioSampleRate = backend.audioSampleRate
        draftAudioBitrateKbps = backend.audioBitrateKbps
        draftReplayDurationSec = backend.replayDurationSec
        draftHotkeyModifiers = backend.hotkeyModifiers
        draftHotkeyVk = backend.hotkeyVk
        draftOutputDir = backend.outputDir
        draftLaunchAtStartup = backend.launchAtStartup
        draftMonitorIndex = backend.monitorIndex
        hotkeyStatus = "Click the recorder, then press a modifier with a key."
        hotkeyStatusError = false
    }

    function resetHotkey() {
        draftHotkeyModifiers = 1
        draftHotkeyVk = 121
        hotkeyStatus = "Shortcut reset to Alt+F10."
        hotkeyStatusError = false
    }

    function modifierLabel(modifiers) {
        let parts = []
        if ((modifiers & 1) !== 0)
            parts.push("Alt")
        if ((modifiers & 2) !== 0)
            parts.push("Ctrl")
        if ((modifiers & 4) !== 0)
            parts.push("Shift")
        if ((modifiers & 8) !== 0)
            parts.push("Win")
        return parts.join("+")
    }

    function keyLabel(vk) {
        if (vk >= 112 && vk <= 135)
            return "F" + (vk - 111)
        if (vk >= 65 && vk <= 90)
            return String.fromCharCode(vk)
        if (vk >= 48 && vk <= 57)
            return String.fromCharCode(vk)
        return "Key " + vk
    }

    function shortcutLabel(modifiers, vk) {
        let mods = modifierLabel(modifiers)
        return mods.length > 0 ? mods + "+" + keyLabel(vk) : keyLabel(vk)
    }

    function virtualKeyFromQtKey(key) {
        if (key >= Qt.Key_F1 && key <= Qt.Key_F24)
            return 112 + (key - Qt.Key_F1)
        if (key >= Qt.Key_A && key <= Qt.Key_Z)
            return 65 + (key - Qt.Key_A)
        if (key >= Qt.Key_0 && key <= Qt.Key_9)
            return 48 + (key - Qt.Key_0)
        return 0
    }

    function modifiersFromQt(modifiers) {
        let bits = 0
        if ((modifiers & Qt.AltModifier) !== 0)
            bits |= 1
        if ((modifiers & Qt.ControlModifier) !== 0)
            bits |= 2
        if ((modifiers & Qt.ShiftModifier) !== 0)
            bits |= 4
        if ((modifiers & Qt.MetaModifier) !== 0)
            bits |= 8
        return bits
    }

    function captureShortcut(event) {
        let vk = virtualKeyFromQtKey(event.key)
        let modifiers = modifiersFromQt(event.modifiers)
        if (vk === 0) {
            hotkeyStatus = "Use an F-key, letter, or number as the primary key."
            hotkeyStatusError = true
            return false
        }
        if (modifiers === 0) {
            hotkeyStatus = "Add Alt, Ctrl, Shift, or Win so the shortcut is less likely to collide."
            hotkeyStatusError = true
            return false
        }
        draftHotkeyModifiers = modifiers
        draftHotkeyVk = vk
        hotkeyStatus = "Shortcut staged: " + shortcutLabel(modifiers, vk) + "."
        hotkeyStatusError = false
        pendingSettings = true
        return true
    }

    component SettingsPane: Item {
        id: settingsPane
        property string title
        property string description
        default property alias paneChildren: contentColumn.data

        Flickable {
            anchors.fill: parent
            contentHeight: contentColumn.implicitHeight + 24
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar { theme: root.uiTheme }

            ColumnLayout {
                id: contentColumn
                width: Math.min(parent.width - 24, 1120)
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: settingsPane.title; color: root.textPrimary; font: root.headingFont }
                    Text { text: settingsPane.description; color: root.textSoft; font: root.bodyFont; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
            }
        }
    }

    component SettingRow: GlassPanel {
        id: row
        property string title
        property string desc
        property bool restartsCapture: false
        default property alias controls: controlSlot.data

        theme: root.uiTheme
        tone: "raised"
        radius: 0
        sheenOpacity: 0
        depth: 0
        opacity: enabled ? 1.0 : 0.45
        Layout.fillWidth: true
        Layout.preferredHeight: root.narrowShell ? 80 : 88

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 12

                Text {
                    text: row.title
                    color: root.textPrimary
                    font: root.bodyFont
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Item {
                    Layout.preferredWidth: root.narrowShell ? 0 : 148
                    Layout.minimumWidth: root.narrowShell ? 0 : 148
                    Layout.maximumWidth: root.narrowShell ? 0 : 148
                    Layout.fillHeight: true
                    visible: !root.narrowShell

                    AppChip {
                        theme: root.uiTheme
                        text: "Restarts capture"
                        accentColor: root.warningYellow
                        visible: row.restartsCapture
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                    }
                }

                RowLayout {
                    id: controlSlot
                    spacing: 6
                    Layout.preferredWidth: root.narrowShell ? 180 : 240
                    Layout.minimumWidth: root.narrowShell ? 120 : 180
                    Layout.maximumWidth: 240
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }
            }

            Text {
                text: row.desc
                color: root.textSoft
                font: root.smallFont
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    component CategoryRow: GlassPanel {
        id: categoryRow
        property string icon
        property string title
        property string subtitle
        property bool selected: false
        signal clicked()

        theme: root.uiTheme
        tone: "raised"
        radius: 0
        sheenOpacity: 0
        depth: 0
        activeFocusOnTab: true
        Layout.fillWidth: true
        Layout.preferredHeight: root.narrowShell ? 46 : 52
        color: selected ? (theme ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.12) : "rgba(37, 99, 235, 0.12)") : (categoryMouse.pressed ? root.pressedBg : (categoryMouse.containsMouse ? root.hoverBg : "transparent"))
        border.color: activeFocus ? root.accent : (selected ? root.borderStrong : (categoryMouse.containsMouse ? root.border : "transparent"))
        border.width: activeFocus ? 2 : 1

        Rectangle {
            visible: selected
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.verticalCenter: parent.verticalCenter
            width: 2
            height: parent.height
            radius: 0
            color: root.accent
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.narrowShell ? 0 : (selected ? 24 : 12)
            anchors.rightMargin: root.narrowShell ? 0 : 10
            spacing: root.narrowShell ? 0 : 10
            
            Item {
                visible: root.narrowShell
                Layout.fillWidth: true
            }

            MonoIcon {
                source: categoryRow.icon
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                tint: selected ? root.controlAccent(categoryRow.icon, false) : root.textSecondary
                glowColor: selected ? root.controlAccent(categoryRow.icon, false) : "transparent"
                glowStrength: selected ? 0.12 : 0.0
                iconOpacity: selected ? 0.96 : 0.72
            }

            Item {
                visible: root.narrowShell
                Layout.fillWidth: true
            }
            ColumnLayout {
                visible: !root.narrowShell
                Layout.fillWidth: true
                spacing: 0
                Text { text: categoryRow.title; color: selected ? root.textPrimary : root.textSecondary; font: root.bodyFont; elide: Text.ElideRight; Layout.fillWidth: true }
                Text { text: categoryRow.subtitle; color: root.textSoft; font: root.smallFont; elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }

        MouseArea {
            id: categoryMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: categoryRow.clicked()
        }

        Keys.onReturnPressed: categoryRow.clicked()
        Keys.onEnterPressed: categoryRow.clicked()
        Keys.onSpacePressed: categoryRow.clicked()

        Accessible.role: Accessible.Button
        Accessible.name: categoryRow.title
        Accessible.description: categoryRow.subtitle
    }

    component FlatCombo: ComboBox {
        id: combo
        Layout.preferredHeight: 36
        font: root.smallFont

        background: GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            radius: 0
            sheenOpacity: 0
            depth: 0
            border.color: combo.activeFocus ? root.accent : root.border
            color: theme ? Qt.rgba(theme.surfaceAlt.r, theme.surfaceAlt.g, theme.surfaceAlt.b, 0.42) : "rgba(13, 21, 39, 0.42)"
        }

        contentItem: Text {
            text: combo.displayText
            color: root.textSecondary
            font: combo.font
            verticalAlignment: Text.AlignVCenter
            leftPadding: 10
            rightPadding: 24
            elide: Text.ElideRight
        }

        delegate: ItemDelegate {
            width: combo.width
            contentItem: Text { text: modelData; color: root.textPrimary; font: root.bodyFont; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: highlighted ? root.hoverBg : "transparent"; radius: 0 }
        }

        popup: Popup {
            y: combo.height + 4
            width: combo.width
            padding: 4
            background: GlassPanel {
                theme: root.uiTheme
                tone: "raised"
                radius: 0
                sheenOpacity: 0
                depth: 0
                border.color: root.borderStrong
            }
            contentItem: ListView {
                implicitHeight: contentHeight
                model: combo.delegateModel
                currentIndex: combo.highlightedIndex
                clip: true
                ScrollBar.vertical: AppScrollBar { theme: root.uiTheme }
            }
        }
    }

    component NumberField: TextField {
        id: numberField
        property int value: 0
        property int from: 0
        property int to: 9999
        signal commit(int nextValue)

        function parsedTextValue() {
            let parsed = parseInt(text)
            return Math.max(from, Math.min(to, isNaN(parsed) ? from : parsed))
        }

        Layout.preferredWidth: root.narrowShell ? 64 : 80
        Layout.preferredHeight: 36
        color: root.textPrimary
        font: root.bodyFont
        horizontalAlignment: Text.AlignHCenter
        selectByMouse: true
        validator: IntValidator { bottom: numberField.from; top: numberField.to }
        Binding {
            target: numberField
            property: "text"
            value: numberField.value.toString()
            when: !numberField.activeFocus
            restoreMode: Binding.RestoreBinding
        }
        background: GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            radius: 0
            sheenOpacity: 0
            depth: 0
            border.color: numberField.activeFocus ? root.accent : root.border
            color: theme ? Qt.rgba(theme.surfaceAlt.r, theme.surfaceAlt.g, theme.surfaceAlt.b, 0.42) : "rgba(13, 21, 39, 0.42)"
        }
        onEditingFinished: {
            commit(parsedTextValue())
        }
        onTextEdited: {
            if (text.length > 0 && acceptableInput) {
                commit(parsedTextValue())
            }
        }
    }

    component StaticField: GlassPanel {
        property string text
        property font font: root.smallFont
        property color accentColor: root.border

        theme: root.uiTheme
        tone: "raised"
        radius: 0
        sheenOpacity: 0
        depth: 0
        color: theme ? Qt.rgba(theme.surfaceAlt.r, theme.surfaceAlt.g, theme.surfaceAlt.b, 0.42) : "rgba(13, 21, 39, 0.42)"
        Layout.preferredHeight: 36
        border.color: accentColor === root.border ? root.border : Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.74)

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 20
            text: parent.text
            color: root.textSecondary
            font: parent.font
            elide: Text.ElideMiddle
        }
    }

    component ShortcutRecorder: AppButton {
        id: recorder
        property int modifiers: 1
        property int vk: 121
        property bool recording: false
        signal recorded(int nextModifiers, int nextVk)

        theme: root.uiTheme
        iconSource: root.iconHotkey
        text: recording ? "Press shortcut" : page.shortcutLabel(modifiers, vk)
        detail: recording ? "Esc cancels" : "Click to change"
        primary: recording
        accentColor: recording ? root.accentPurple : root.accent
        tooltip: "Record save replay shortcut"
        Layout.preferredWidth: root.narrowShell ? 160 : 260
        Layout.preferredHeight: 38

        onClicked: {
            recording = true
            page.hotkeyStatus = "Listening. Press Alt, Ctrl, Shift, or Win with a key."
            page.hotkeyStatusError = false
            forceActiveFocus()
        }

        Keys.onPressed: function(event) {
            if (!recording)
                return
            if (event.key === Qt.Key_Escape) {
                recording = false
                page.hotkeyStatus = "Shortcut recording canceled."
                page.hotkeyStatusError = false
                event.accepted = true
                return
            }
            if (page.captureShortcut(event)) {
                recorded(page.draftHotkeyModifiers, page.draftHotkeyVk)
                recording = false
            }
            event.accepted = true
        }

        Accessible.name: recording ? "Recording save replay shortcut" : "Save replay shortcut " + page.shortcutLabel(modifiers, vk)
        Accessible.description: recording ? "Press a modifier with an F-key, letter, or number. Escape cancels." : "Click to record a new shortcut."
    }

    component UnitText: Text {
        color: root.textSoft
        font: root.smallFont
    }

    component ActionFieldButton: AppButton {
        property string iconPath
        theme: root.uiTheme
        iconSource: iconPath
        accentColor: root.controlAccent(iconPath, false)
        Layout.preferredWidth: 88
        Layout.preferredHeight: 36
    }

    component SaveButton: AppButton {
        theme: root.uiTheme
        iconSource: root.iconSave
        text: "Save"
        primary: true
        Layout.preferredWidth: 110
        Layout.preferredHeight: 38
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: root.border
    }
}
