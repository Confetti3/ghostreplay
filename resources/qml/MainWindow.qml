import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    Theme { id: themeTokens }

    title: "Ghost Replay"
    width: 1280
    height: 760
    minimumWidth: 1040
    minimumHeight: 720
    visible: false
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    readonly property QtObject uiTheme: themeTokens
    readonly property int windowRadius: windowChrome.maximized ? 0 : 13
    readonly property bool compactShell: width < 1140

    readonly property color shell: themeTokens.shell
    readonly property color rail: themeTokens.rail
    readonly property color panel: themeTokens.surface
    readonly property color panelAlt: themeTokens.surfaceAlt
    readonly property color panelRaised: themeTokens.surfaceRaised
    readonly property color panelStrong: themeTokens.surfaceStrong
    readonly property color hoverBg: themeTokens.hover
    readonly property color pressedBg: themeTokens.pressed
    readonly property color border: themeTokens.border
    readonly property color borderStrong: themeTokens.borderStrong
    readonly property color textPrimary: themeTokens.textPrimary
    readonly property color textSecondary: themeTokens.textSecondary
    readonly property color textMuted: themeTokens.textMuted
    readonly property color textSoft: themeTokens.textSoft
    readonly property color accent: themeTokens.accent
    readonly property color accentHover: themeTokens.accentStrong
    readonly property color accentStrong: themeTokens.accentStrong
    readonly property color accentMuted: themeTokens.accentSoft
    readonly property color accentBlue: themeTokens.accentBlue
    readonly property color accentCyan: themeTokens.accentCyan
    readonly property color accentGreen: themeTokens.accentGreen
    readonly property color accentPurple: themeTokens.accentPurple
    readonly property color accentViolet: themeTokens.accentPurple
    readonly property color successGreen: themeTokens.accentGreen
    readonly property color warningYellow: themeTokens.accentAmber
    readonly property color recordRed: themeTokens.accentRed
    readonly property color dimRed: themeTokens.dangerSoft
    readonly property color glassLine: themeTokens.borderStrong
    readonly property color overlay: themeTokens.overlay
    readonly property color inactive: themeTokens.inactive

    readonly property string iconAudio: "qrc:/icons/ui/audio.svg"
    readonly property string iconBack: "qrc:/icons/ui/back.svg"
    readonly property string iconBuffer: "qrc:/icons/ui/buffer.svg"
    readonly property string iconClipboard: "qrc:/icons/ui/clipboard.svg"
    readonly property string iconClips: "qrc:/icons/ui/clips.svg"
    readonly property string iconDelete: "qrc:/icons/ui/delete.svg"
    readonly property string iconDesktop: "qrc:/icons/ui/desktop.svg"
    readonly property string iconEdit: "qrc:/icons/ui/edit.svg"
    readonly property string iconExport: "qrc:/icons/ui/export.svg"
    readonly property string iconFolder: "qrc:/icons/ui/folder.svg"
    readonly property string iconGrid: "qrc:/icons/ui/grid.svg"
    readonly property string iconHotkey: "qrc:/icons/ui/hotkey.svg"
    readonly property string iconList: "qrc:/icons/ui/list.svg"
    readonly property string iconOpen: "qrc:/icons/ui/open.svg"
    readonly property string iconOutput: "qrc:/icons/ui/output.svg"
    readonly property string iconOverview: "qrc:/icons/ui/overview.svg"
    readonly property string iconPause: "qrc:/icons/ui/pause.svg"
    readonly property string iconPlay: "qrc:/icons/ui/play.svg"
    readonly property string iconRefresh: "qrc:/icons/ui/refresh.svg"
    readonly property string iconReveal: "qrc:/icons/ui/reveal.svg"
    readonly property string iconSave: "qrc:/icons/ui/save.svg"
    readonly property string iconSearch: "qrc:/icons/ui/search.svg"
    readonly property string iconSettings: "qrc:/icons/ui/settings.svg"
    readonly property string iconTrimIn: "qrc:/icons/ui/trim-in.svg"
    readonly property string iconTrimOut: "qrc:/icons/ui/trim-out.svg"
    readonly property string iconVideo: "qrc:/icons/ui/video.svg"
    readonly property string iconVolume: "qrc:/icons/ui/volume.svg"

    property int currentView: 0
    property int previousView: 1
    property var editorClip: null
    FontLoader { source: "qrc:/fonts/SpaceGrotesk-wght.ttf" }
    FontLoader { source: "qrc:/fonts/Inter-opsz-wght.ttf" }

    property font titleFont: themeTokens.titleFont
    property font headingFont: themeTokens.headingFont
    property font bodyFont: themeTokens.bodyFont
    property font smallFont: themeTokens.smallFont
    property font monoFont: themeTokens.monoFont
    property font iconFont: themeTokens.iconFont

    onClosing: function(close) {
        close.accepted = false
        backend.hideToTray()
    }

    palette {
        base: panelRaised
        alternateBase: panel
        button: panel
        buttonText: textPrimary
        text: textPrimary
        window: shell
        windowText: textPrimary
        highlight: accent
        highlightedText: shell
        toolTipBase: panelRaised
        toolTipText: textPrimary
    }

    Rectangle {
        id: shellFrame
        anchors.fill: parent
        radius: root.windowRadius
        color: root.shell
        border.color: windowChrome.maximized ? "transparent" : Qt.rgba(0.62, 0.76, 0.92, 0.18)
        border.width: windowChrome.maximized ? 0 : 1
        clip: true
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0C1110" }
            GradientStop { position: 0.36; color: "#070B0A" }
            GradientStop { position: 1.0; color: "#030504" }
        }

        Repeater {
            model: 7
            Rectangle {
                width: shellFrame.width * 1.2
                height: 1
                x: -shellFrame.width * 0.1
                y: 56 + index * Math.max(34, shellFrame.height / 16)
                rotation: -8
                color: Qt.rgba(0.56, 0.97, 0.78, index % 3 === 0 ? 0.014 : 0.006)
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            GlassPanel {
                id: titleBar
                theme: themeTokens
                tone: "chrome"
                radius: root.windowRadius
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                border.width: 0

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onPressed: if (mouse.button === Qt.LeftButton) windowChrome.startSystemMove()
                    onDoubleClicked: if (mouse.button === Qt.LeftButton) windowChrome.maximizeRestore()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 0
                    spacing: 12

                    RowLayout {
                        spacing: 12
                        Layout.preferredWidth: 240

                        AppLogo {
                            theme: themeTokens
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                        }

                        Text {
                            text: "Ghost Replay"
                            color: root.textPrimary
                            font: Qt.font({ family: "Segoe UI Variable Display", pixelSize: 18, weight: Font.DemiBold })
                        }
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        spacing: 8
                        AppChip {
                            theme: themeTokens
                            text: backend.captureAvailable ? (backend.recording ? "Live capture" : "Capture paused") : "Capture unavailable"
                            accentColor: backend.captureAvailable
                                ? (backend.recording ? root.accentBlue : root.inactive)
                                : root.recordRed
                            strong: true
                        }
                        AppChip {
                            theme: themeTokens
                            text: (backend.captureWidth > 0 ? backend.captureWidth + "x" + backend.captureHeight : "Native") + " " + backend.captureFps
                            accentColor: root.accentCyan
                            strong: true
                        }
                    }

                    RowLayout {
                        spacing: 0
                        WindowButton { glyph: "\uE921"; onClicked: windowChrome.minimize() }
                        WindowButton { glyph: windowChrome.maximized ? "\uE923" : "\uE922"; onClicked: windowChrome.maximizeRestore() }
                        WindowButton { glyph: "\uE8BB"; hoverColor: "#65212B"; danger: true; onClicked: backend.hideToTray() }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 14

                    GlassPanel {
                        id: navRail
                        theme: themeTokens
                        tone: "rail"
                        Layout.preferredWidth: root.compactShell ? 88 : 250
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 7
                                Text {
                                    visible: !root.compactShell
                                    text: "WORKSPACE"
                                    color: root.textMuted
                                    font: root.smallFont
                                }

                                GlassPanel {
                                    theme: themeTokens
                                    tone: "raised"
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40
                                    color: Qt.rgba(0.055, 0.086, 0.086, 0.58)
                                    border.color: Qt.rgba(root.border.r, root.border.g, root.border.b, 0.48)

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 10

                                        MonoIcon {
                                            source: root.iconDesktop
                                            Layout.preferredWidth: 18
                                            Layout.preferredHeight: 18
                                            tint: root.accentBlue
                                            glowColor: root.accentBlue
                                            glowStrength: 0.13
                                            treatment: "featured"
                                            iconOpacity: 0.98
                                        }

                                        Text {
                                            visible: !root.compactShell
                                            text: "Desktop replay"
                                            color: root.textPrimary
                                            font: root.bodyFont
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            Text {
                                visible: !root.compactShell
                                text: "CAPTURE CONTROLS"
                                color: root.textMuted
                                font: root.smallFont
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ActionButton {
                                    icon: root.iconSave
                                    label: "Save replay"
                                    detail: backend.captureAvailable ? shortcutText() : "Unavailable"
                                    primary: true
                                    enabled: backend.captureAvailable
                                    onClicked: backend.saveClip()
                                }

                                ActionButton {
                                    icon: backend.captureAvailable ? (backend.recording ? root.iconPause : root.iconPlay) : root.iconRefresh
                                    label: backend.captureAvailable ? (backend.recording ? "Pause capture" : "Resume capture") : "Retry capture"
                                    detail: backend.captureAvailable ? (backend.recording ? "Stop" : "Resume") : "Health"
                                    accentColor: backend.captureAvailable ? (backend.recording ? root.recordRed : root.accent) : root.warningYellow
                                    onClicked: backend.captureAvailable ? backend.toggleRecording() : backend.retryCapture()
                                }
                            }

                            Divider {}

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Text {
                                    visible: !root.compactShell
                                    text: "NAVIGATION"
                                    color: root.textMuted
                                    font: root.smallFont
                                }

                                NavRow { icon: root.iconOverview; label: "Overview"; detail: "Capture state"; selected: root.currentView === 0; onClicked: root.goToView(0) }
                                NavRow { icon: root.iconClips; label: "Clips"; detail: backend.clipLibrary.length + " saved"; selected: root.currentView === 1 || root.currentView === 3; onClicked: root.goToView(1, true) }
                                NavRow { icon: root.iconSettings; label: "Settings"; detail: "Profile and output"; selected: root.currentView === 2; onClicked: root.goToView(2) }
                            }

                            Divider {}

                            ColumnLayout {
                                visible: !root.compactShell
                                Layout.fillWidth: true
                                spacing: 10

                                Text {
                                    text: "RUNTIME"
                                    color: root.textMuted
                                    font: root.smallFont
                                }

                                InfoLine { label: "State"; value: backend.captureAvailable ? (backend.recording ? "Recording" : "Paused") : "Unavailable"; accentColor: backend.captureAvailable ? (backend.recording ? root.accent : root.inactive) : root.recordRed }
                                InfoLine { label: "Buffer"; value: backend.bufferSeconds + "s / " + backend.bufferPackets + " packets"; accentColor: root.accentStrong }
                                InfoLine { label: "Audio"; value: backend.audioActive ? "Loopback on" : "Disabled"; accentColor: backend.audioActive ? root.successGreen : root.inactive }
                                InfoLine { label: "Codec"; value: backend.captureCodec.toUpperCase() + "  CQP " + backend.captureCqp; accentColor: backend.captureAvailable ? root.textSecondary : root.warningYellow }
                            }

                            Item { Layout.fillHeight: true }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                SmallIconButton { icon: root.iconSettings; tooltip: "Open settings"; onClicked: root.goToView(2) }
                                SmallIconButton { icon: root.iconRefresh; tooltip: "Refresh clips"; onClicked: backend.requestRefresh() }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }

                    GlassPanel {
                        theme: themeTokens
                        tone: "surface"
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        StackLayout {
                            anchors.fill: parent
                            currentIndex: root.currentView

                            DashboardPage {}
                            ClipsPage { id: clipsPage; onEditClipRequested: root.openClipEditor(clip) }
                            SettingsPage {}
                            ClipEditorPage {
                                id: clipEditorPage
                                onDoneRequested: {
                                    root.currentView = root.previousView
                                    backend.requestRefresh()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: resizeTop
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: windowChrome.maximized ? 0 : 6
        color: "transparent"
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: if (mouse.button === Qt.LeftButton) windowChrome.startSystemResize(Qt.TopEdge)
        }
    }

    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: windowChrome.maximized ? 0 : 6
        color: "transparent"
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: if (mouse.button === Qt.LeftButton) windowChrome.startSystemResize(Qt.BottomEdge)
        }
    }

    Rectangle {
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: windowChrome.maximized ? 0 : 6
        color: "transparent"
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: if (mouse.button === Qt.LeftButton) windowChrome.startSystemResize(Qt.LeftEdge)
        }
    }

    Rectangle {
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
        width: windowChrome.maximized ? 0 : 6
        color: "transparent"
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: if (mouse.button === Qt.LeftButton) windowChrome.startSystemResize(Qt.RightEdge)
        }
    }

    ResizeCorner {
        anchors.left: parent.left
        anchors.top: parent.top
        cursor: Qt.SizeFDiagCursor
        edges: Qt.LeftEdge | Qt.TopEdge
    }

    ResizeCorner {
        anchors.right: parent.right
        anchors.top: parent.top
        cursor: Qt.SizeBDiagCursor
        edges: Qt.RightEdge | Qt.TopEdge
    }

    ResizeCorner {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        cursor: Qt.SizeBDiagCursor
        edges: Qt.LeftEdge | Qt.BottomEdge
    }

    ResizeCorner {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursor: Qt.SizeFDiagCursor
        edges: Qt.RightEdge | Qt.BottomEdge
    }

    Popup {
        id: errorPopup
        property string severity: "error"
        modal: false
        width: Math.min(420, Math.max(280, root.width - 48))
        x: Math.max(24, root.width - width - 24)
        y: 66
        padding: 12
        background: GlassPanel {
            theme: themeTokens
            tone: errorPopup.severity === "success" ? "raised" : "surface"
            border.color: errorPopup.severity === "success"
                ? root.successGreen
                : (errorPopup.severity === "info" ? root.accent : root.recordRed)
            color: errorPopup.severity === "success"
                ? Qt.rgba(0.09, 0.22, 0.14, 0.94)
                : (errorPopup.severity === "info" ? Qt.rgba(0.07, 0.15, 0.23, 0.94) : Qt.rgba(0.25, 0.09, 0.12, 0.96))
        }
        contentItem: Text {
            id: errText
            width: errorPopup.width - 28
            wrapMode: Text.Wrap
            color: root.textPrimary
            font: root.bodyFont
        }
        Timer { id: errTimer; interval: 5000; onTriggered: errorPopup.close() }
        function show(msg, nextSeverity) {
            severity = nextSeverity || "error"
            errText.text = msg
            open()
            errTimer.restart()
        }
    }

    Connections {
        target: backend
        function onUserNotification(msg, severity) { errorPopup.show(msg, severity) }
    }

    function shortcutText() {
        let parts = []
        let m = backend.hotkeyModifiers
        if (m & 1) parts.push("Alt")
        if (m & 2) parts.push("Ctrl")
        if (m & 4) parts.push("Shift")
        if (m & 8) parts.push("Win")
        parts.push("F" + (backend.hotkeyVk - 111))
        return parts.join("+")
    }

    function controlAccent(iconSource, danger) {
        if (danger)
            return recordRed
        if (iconSource === iconSave || iconSource === iconOverview || iconSource === iconDesktop || iconSource === iconSettings || iconSource === iconHotkey)
            return accentBlue
        if (iconSource === iconFolder || iconSource === iconReveal || iconSource === iconOpen || iconSource === iconOutput)
            return accentCyan
        if (iconSource === iconClipboard || iconSource === iconVideo || iconSource === iconTrimIn || iconSource === iconTrimOut)
            return accentPurple
        if (iconSource === iconBuffer || iconSource === iconRefresh || iconSource === iconPlay || iconSource === iconClips)
            return accentGreen
        if (iconSource === iconAudio || iconSource === iconVolume || iconSource === iconPause)
            return warningYellow
        return accentHover
    }

    function openClipEditor(clip) {
        if (!clip)
            return
        editorClip = clip
        previousView = 1
        currentView = 3
        clipEditorPage.openForClip(clip)
    }

    function goToView(view, refreshClips) {
        if (currentView === 3 && view !== 3)
            clipEditorPage.closeEditor()
        currentView = view
        if (refreshClips)
            backend.requestRefresh()
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: root.border
        opacity: 0.7
    }

    component ResizeCorner: Rectangle {
        property int edges
        property int cursor
        width: windowChrome.maximized ? 0 : 10
        height: windowChrome.maximized ? 0 : 10
        color: "transparent"

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: parent.cursor
            onPressed: if (mouse.button === Qt.LeftButton) windowChrome.startSystemResize(parent.edges)
        }
    }

    component WindowButton: Rectangle {
        property string glyph
        property color hoverColor: root.hoverBg
        property bool danger: false
        signal clicked()

        Layout.preferredWidth: 48
        Layout.preferredHeight: titleBar.height
        color: mouse.pressed ? root.pressedBg : mouse.containsMouse ? hoverColor : "transparent"

        Text {
            anchors.centerIn: parent
            text: parent.glyph
            color: danger && mouse.containsMouse ? "#FFFFFF" : root.textSecondary
            font: Qt.font({ family: "Segoe Fluent Icons", pixelSize: 10 })
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    component ActionButton: Rectangle {
        id: actionButton
        property string icon
        property string label
        property string detail
        property bool primary: false
        property color accentColor: root.accent
        signal clicked()

        Layout.fillWidth: true
        Layout.preferredHeight: 42
        activeFocusOnTab: true
        radius: themeTokens.radiusMd
        opacity: enabled ? 1.0 : 0.48
        color: enabled && mouse.pressed ? root.pressedBg : enabled && mouse.containsMouse ? Qt.rgba(0.10, 0.16, 0.23, 0.94) : Qt.rgba(0.03, 0.05, 0.09, 0.46)
        border.color: activeFocus ? accentColor : (primary ? (enabled && mouse.containsMouse ? accentColor : root.borderStrong) : (enabled && mouse.containsMouse ? root.borderStrong : root.border))
        border.width: activeFocus ? 2 : 1

        Rectangle {
            visible: primary
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 2
            height: 20
            radius: 1
            color: actionButton.accentColor
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: primary ? 16 : 12
            anchors.rightMargin: 12
            spacing: 9

            MonoIcon {
                source: actionButton.icon
                Layout.preferredWidth: actionButton.primary ? 18 : 17
                Layout.preferredHeight: actionButton.primary ? 18 : 17
                tint: actionButton.primary ? actionButton.accentColor : root.textSecondary
                glowColor: actionButton.primary ? actionButton.accentColor : "transparent"
                glowStrength: actionButton.primary ? 0.12 : 0.0
                treatment: actionButton.primary ? "featured" : "plain"
                iconOpacity: primary ? 0.98 : 0.84
            }

            Text {
                visible: !root.compactShell
                text: actionButton.label
                color: root.textPrimary
                font: root.bodyFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                visible: !root.compactShell
                text: actionButton.detail
                color: actionButton.primary ? root.accentHover : root.textSoft
                font: root.smallFont
                elide: Text.ElideRight
                Layout.maximumWidth: 84
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            enabled: parent.enabled
            hoverEnabled: true
            cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: parent.clicked()
        }

        Keys.onReturnPressed: if (enabled) actionButton.clicked()
        Keys.onEnterPressed: if (enabled) actionButton.clicked()
        Keys.onSpacePressed: if (enabled) actionButton.clicked()

        Accessible.role: Accessible.Button
        Accessible.name: actionButton.label
        Accessible.description: actionButton.detail
    }

    component NavRow: Rectangle {
        id: navRow
        property string icon
        property string label
        property string detail
        property bool selected: false
        signal clicked()

        Layout.fillWidth: true
        Layout.preferredHeight: 48
        activeFocusOnTab: true
        radius: themeTokens.radiusMd
        color: selected ? Qt.rgba(0.09, 0.17, 0.27, 0.82) : (mouse.pressed ? root.pressedBg : (mouse.containsMouse ? Qt.rgba(0.08, 0.13, 0.19, 0.50) : "transparent"))
        border.color: activeFocus ? root.accent : (selected ? root.borderStrong : (mouse.containsMouse ? root.border : "transparent"))
        border.width: activeFocus ? 2 : 1

        Rectangle {
            visible: selected
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 2
            height: 18
            radius: 1
            color: root.accent
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 10
            spacing: 10

            MonoIcon {
                source: navRow.icon
                Layout.preferredWidth: navRow.selected ? 20 : 18
                Layout.preferredHeight: navRow.selected ? 20 : 18
                tint: navRow.selected ? (navRow.icon === root.iconOverview ? root.textPrimary : root.controlAccent(navRow.icon, false)) : root.textSecondary
                glowColor: navRow.selected ? root.controlAccent(navRow.icon, false) : "transparent"
                glowStrength: navRow.selected ? 0.15 : 0.0
                treatment: navRow.selected ? "featured" : "plain"
                iconOpacity: navRow.selected ? 0.98 : 0.84
            }

            ColumnLayout {
                visible: !root.compactShell
                Layout.fillWidth: true
                spacing: 0

                Text {
                    text: navRow.label
                    color: navRow.selected ? root.textPrimary : root.textSecondary
                    font: root.bodyFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: navRow.detail
                    color: root.textSoft
                    font: root.smallFont
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }

        Keys.onReturnPressed: navRow.clicked()
        Keys.onEnterPressed: navRow.clicked()
        Keys.onSpacePressed: navRow.clicked()

        Accessible.role: Accessible.Button
        Accessible.name: navRow.label
        Accessible.description: navRow.detail
    }

    component InfoLine: RowLayout {
        property string label
        property string value
        property color accentColor: root.accent
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 7
            Layout.preferredHeight: 7
            radius: 4
            color: parent.accentColor
        }

        Text {
            text: parent.label
            color: root.textSoft
            font: root.smallFont
            Layout.preferredWidth: 48
        }

        Text {
            text: parent.value
            color: root.textSecondary
            font: root.smallFont
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    component SmallIconButton: Rectangle {
        id: smallIconButton
        property string icon
        property string tooltip
        signal clicked()

        Layout.preferredWidth: 34
        Layout.preferredHeight: 34
        activeFocusOnTab: true
        radius: themeTokens.radiusMd
        color: mouse.pressed ? root.pressedBg : mouse.containsMouse ? root.hoverBg : Qt.rgba(0.05, 0.08, 0.12, 0.56)
        border.color: activeFocus ? root.controlAccent(smallIconButton.icon, false) : (mouse.containsMouse ? root.borderStrong : root.border)
        border.width: activeFocus ? 2 : 1

        MonoIcon {
            anchors.centerIn: parent
            source: smallIconButton.icon
            width: 15
            height: 15
            tint: mouse.containsMouse ? root.controlAccent(smallIconButton.icon, false) : root.textSecondary
            glowColor: "transparent"
            glowStrength: 0.0
            treatment: "compact"
            iconOpacity: mouse.containsMouse ? 0.92 : 0.72
        }

        ToolTip.visible: (mouse.containsMouse || smallIconButton.activeFocus) && smallIconButton.tooltip.length > 0
        ToolTip.text: smallIconButton.tooltip
        Accessible.role: Accessible.Button
        Accessible.name: smallIconButton.tooltip

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: smallIconButton.clicked()
        }

        Keys.onReturnPressed: smallIconButton.clicked()
        Keys.onEnterPressed: smallIconButton.clicked()
        Keys.onSpacePressed: smallIconButton.clicked()
    }
}
