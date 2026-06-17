import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page

    background: Rectangle { color: "transparent" }

    property var filteredModel: []
    property bool gridMode: false
    property var selectedClip: filteredModel.length > 0 ? filteredModel[0] : null
    property var contextClip: null
    property string pendingDeletePath: ""
    property string pendingDeleteName: ""
    readonly property bool compactList: page.width < 900
    readonly property int listLengthWidth: 56
    readonly property int listGameWidth: compactList ? 0 : 112
    readonly property int listSizeWidth: compactList ? 0 : 68
    readonly property int listDateWidth: compactList ? 0 : 126
    readonly property int listActionsWidth: compactList ? 128 : 144
    readonly property int scrollGutter: 20
    signal editClipRequested(var clip)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        GlassPanel {
            theme: root.uiTheme
            tone: "surface"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            text: "Clips"
                            color: root.textPrimary
                            font: root.headingFont
                        }

                        Text {
                            text: "Find saved moments, open the file, or make a shareable export."
                            color: root.textSoft
                            font: root.bodyFont
                        }
                    }

                    AppChip {
                        theme: root.uiTheme
                        text: filteredModel.length + " visible"
                        accentColor: root.accentBlue
                        strong: true
                    }

                    AppChip {
                        visible: backend.exportBusy
                        theme: root.uiTheme
                        text: backend.exportStatus || "Exporting"
                        accentColor: root.accentGreen
                        strong: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    SearchField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholder: "Search clips, games, dates"
                        onTextChanged: applyFilter()
                    }

                    FlatCombo {
                        id: gameFilter
                        Layout.preferredWidth: 178
                        model: getGameFilters()
                        onCurrentIndexChanged: applyFilter()
                    }

                    SegmentedButton { active: !page.gridMode; iconPath: root.iconList; text: "List"; tooltip: "Show clips as a list"; onClicked: page.gridMode = false }
                    SegmentedButton { active: page.gridMode; iconPath: root.iconGrid; text: "Grid"; tooltip: "Show clips as a grid"; onClicked: page.gridMode = true }
                    ToolAction { iconPath: root.iconRefresh; text: "Refresh"; onClicked: backend.requestRefresh() }
                    ToolAction { iconPath: root.iconFolder; text: "Folder"; onClicked: backend.openOutputDir() }
                }

                GlassPanel {
                    theme: root.uiTheme
                    tone: "alt"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StackLayout {
                        anchors.fill: parent
                        currentIndex: page.gridMode ? 1 : 0

                        ListView {
                            id: clipList
                            clip: true
                            spacing: 0
                            model: filteredModel
                            boundsBehavior: Flickable.StopAtBounds

                            header: GlassPanel {
                                theme: root.uiTheme
                                tone: "raised"
                                width: Math.max(0, clipList.width - page.scrollGutter)
                                height: 38
                                radius: 0
                                border.color: root.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10
                                    HeaderText { text: "Clip"; Layout.fillWidth: true }
                                    HeaderText { text: "Length"; Layout.preferredWidth: page.listLengthWidth; Layout.minimumWidth: page.listLengthWidth; Layout.maximumWidth: page.listLengthWidth }
                                    HeaderText { text: "Game"; visible: !page.compactList; Layout.preferredWidth: page.listGameWidth; Layout.minimumWidth: page.listGameWidth; Layout.maximumWidth: page.listGameWidth }
                                    HeaderText { text: "Size"; visible: !page.compactList; Layout.preferredWidth: page.listSizeWidth; Layout.minimumWidth: page.listSizeWidth; Layout.maximumWidth: page.listSizeWidth }
                                    HeaderText { text: "Date"; visible: !page.compactList; Layout.preferredWidth: page.listDateWidth; Layout.minimumWidth: page.listDateWidth; Layout.maximumWidth: page.listDateWidth }
                                    Item { Layout.preferredWidth: page.listActionsWidth; Layout.minimumWidth: page.listActionsWidth; Layout.maximumWidth: page.listActionsWidth }
                                }
                            }

                            delegate: GlassPanel {
                                required property var modelData

                                theme: root.uiTheme
                                tone: "surface"
                                width: Math.max(0, clipList.width - page.scrollGutter)
                                height: 72
                                radius: 0
                                color: isSelected(modelData.path) ? Qt.rgba(0.055, 0.110, 0.095, 0.78) : (listMouse.containsMouse ? Qt.rgba(0.08, 0.12, 0.12, 0.78) : root.panel)
                                border.color: isSelected(modelData.path) ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.34) : root.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10

                                    AppThumbnail {
                                        theme: root.uiTheme
                                        Layout.preferredWidth: 82
                                        Layout.preferredHeight: 46
                                        clipPath: modelData.path
                                        fallbackText: "No preview"
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 140
                                        spacing: 2

                                        Text {
                                            text: modelData.name
                                            color: root.textPrimary
                                            font: root.bodyFont
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            text: modelData.path
                                            color: root.textSoft
                                            font: root.monoFont
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Text { text: modelData.duration || ""; color: root.textSecondary; font: root.smallFont; elide: Text.ElideRight; Layout.preferredWidth: page.listLengthWidth; Layout.minimumWidth: page.listLengthWidth; Layout.maximumWidth: page.listLengthWidth }
                                    Text { visible: !page.compactList; text: modelData.game || "Desktop"; color: root.textSecondary; font: root.smallFont; Layout.preferredWidth: page.listGameWidth; Layout.minimumWidth: page.listGameWidth; Layout.maximumWidth: page.listGameWidth; elide: Text.ElideRight }
                                    Text { visible: !page.compactList; text: modelData.size || ""; color: root.textSoft; font: root.smallFont; elide: Text.ElideRight; Layout.preferredWidth: page.listSizeWidth; Layout.minimumWidth: page.listSizeWidth; Layout.maximumWidth: page.listSizeWidth }
                                    Text { visible: !page.compactList; text: modelData.date || ""; color: root.textSoft; font: root.smallFont; Layout.preferredWidth: page.listDateWidth; Layout.minimumWidth: page.listDateWidth; Layout.maximumWidth: page.listDateWidth; elide: Text.ElideRight }

                                    RowLayout {
                                        Layout.preferredWidth: page.listActionsWidth
                                        Layout.minimumWidth: page.listActionsWidth
                                        Layout.maximumWidth: page.listActionsWidth
                                        spacing: 6
                                        TinyAction { iconPath: root.iconEdit; text: "Share"; tooltip: "Trim and export this clip"; onClicked: { page.selectedClip = modelData; page.editClipRequested(modelData) } }
                                        TinyAction { compact: true; iconPath: root.iconOpen; text: ""; tooltip: "More clip actions"; onClicked: { page.selectedClip = modelData; let p = mapToItem(page, width, height); page.openClipContext(modelData, p.x, p.y) } }
                                    }
                                }

                                MouseArea {
                                    id: listMouse
                                    z: -1
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: function(mouse) {
                                        page.selectedClip = modelData
                                        if (mouse.button === Qt.RightButton) {
                                            let p = listMouse.mapToItem(page, mouse.x, mouse.y)
                                            page.openClipContext(modelData, p.x, p.y)
                                        }
                                    }
                                    onDoubleClicked: function(mouse) {
                                        if (mouse.button === Qt.LeftButton)
                                            backend.openClip(modelData.path)
                                    }
                                }
                            }

                            EmptyState {
                                anchors.centerIn: parent
                                visible: clipList.count === 0
                                maxWidth: Math.min(clipList.width - 80, 520)
                            }

                            ScrollBar.vertical: AppScrollBar {
                                theme: root.uiTheme
                                x: clipList.width - width - 4
                            }
                        }

                        GridView {
                            id: clipGrid
                            readonly property int usableWidth: Math.max(1, width - page.scrollGutter)
                            readonly property int columnCount: Math.max(1, Math.floor(usableWidth / 236))
                            readonly property real cardWidth: Math.floor(usableWidth / columnCount) - 12

                            anchors.margins: 12
                            clip: true
                            model: filteredModel
                            cellWidth: Math.floor(usableWidth / columnCount)
                            cellHeight: 190
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: GlassPanel {
                                required property var modelData

                                theme: root.uiTheme
                                tone: "surface"
                                width: clipGrid.cardWidth
                                height: clipGrid.cellHeight - 12
                                color: isSelected(modelData.path) ? Qt.rgba(0.055, 0.110, 0.095, 0.78) : (gridMouse.containsMouse ? Qt.rgba(0.08, 0.12, 0.12, 0.80) : root.panel)
                                border.color: isSelected(modelData.path) ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.34) : root.border

                                AppThumbnail {
                                    theme: root.uiTheme
                                    anchors { left: parent.left; right: parent.right; top: parent.top }
                                    height: Math.min(124, Math.max(108, parent.width * 0.48))
                                    radius: 0
                                    clipPath: modelData.path
                                    fallbackText: "No preview"
                                }

                                ColumnLayout {
                                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 12 }
                                    spacing: 3

                                    Text { text: modelData.name; color: root.textPrimary; font: root.bodyFont; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text { text: modelData.date || ""; color: root.textSoft; font: root.smallFont; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text { text: (modelData.game || "Desktop") + "  " + (modelData.duration || "") + "  " + (modelData.size || ""); color: root.textSecondary; font: root.smallFont; elide: Text.ElideRight; Layout.fillWidth: true }
                                }

                                RowLayout {
                                    visible: gridMouse.containsMouse || isSelected(modelData.path)
                                    anchors { top: parent.top; right: parent.right; topMargin: 10; rightMargin: 10 }
                                    spacing: 6
                                    TinyAction { iconPath: root.iconEdit; text: "Share"; tooltip: "Trim and export"; onClicked: { page.selectedClip = modelData; page.editClipRequested(modelData) } }
                                    TinyAction { compact: true; iconPath: root.iconOpen; text: ""; tooltip: "More actions"; onClicked: { page.selectedClip = modelData; let p = mapToItem(page, width, height); page.openClipContext(modelData, p.x, p.y) } }
                                }

                                MouseArea {
                                    id: gridMouse
                                    z: -1
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: function(mouse) {
                                        page.selectedClip = modelData
                                        if (mouse.button === Qt.RightButton) {
                                            let p = gridMouse.mapToItem(page, mouse.x, mouse.y)
                                            page.openClipContext(modelData, p.x, p.y)
                                        }
                                    }
                                    onDoubleClicked: function(mouse) {
                                        if (mouse.button === Qt.LeftButton)
                                            backend.openClip(modelData.path)
                                    }
                                }
                            }

                            EmptyState {
                                anchors.centerIn: parent
                                visible: clipGrid.count === 0
                                maxWidth: Math.min(clipGrid.width - 80, 520)
                            }

                            ScrollBar.vertical: AppScrollBar {
                                theme: root.uiTheme
                                x: clipGrid.width - width - 4
                            }
                        }
                    }
                }
            }
        }

        GlassPanel {
            theme: root.uiTheme
            tone: "rail"
            Layout.preferredWidth: page.width >= 1180 ? 320 : 0
            Layout.minimumWidth: page.width >= 1180 ? 320 : 0
            Layout.maximumWidth: page.width >= 1180 ? 320 : 0
            Layout.fillHeight: true
            visible: page.width >= 1180

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text { text: "Selection"; color: root.textPrimary; font: root.titleFont }
                Text { text: selectedClip ? "Clip details and actions" : "Choose a clip to inspect it."; color: root.textSoft; font: root.smallFont }

                GlassPanel {
                    theme: root.uiTheme
                    tone: "alt"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 178

                    AppThumbnail {
                        theme: root.uiTheme
                        anchors.fill: parent
                        clipPath: selectedClip ? selectedClip.path : ""
                        fallbackText: selectedClip ? "No preview" : "No clip selected"
                        radius: parent.radius
                    }
                }

                InspectorBlock {
                    title: "Metadata"
                    visible: !!selectedClip
                    rows: selectedClip ? [
                        { label: "Name", value: selectedClip.name || "" },
                        { label: "Game", value: selectedClip.game || "Desktop" },
                        { label: "Date", value: selectedClip.date || "" },
                        { label: "Size", value: selectedClip.size || "" },
                        { label: "Length", value: selectedClip.duration || "" },
                        { label: "Video", value: (selectedClip.resolution || "") + (selectedClip.fps ? "  " + Math.round(selectedClip.fps) + " fps" : "") },
                        { label: "Audio", value: selectedClip.hasAudio ? "Included" : "None" }
                    ] : []
                }

                InspectorBlock {
                    title: "Path"
                    visible: !!selectedClip
                    rows: selectedClip ? [
                        { label: "File", value: selectedClip.path || "" }
                    ] : []
                }

                ColumnLayout {
                    visible: !!selectedClip
                    Layout.fillWidth: true
                    spacing: 8
                    ToolAction { wide: true; iconPath: root.iconEdit; text: "Share clip"; onClicked: if (selectedClip) page.editClipRequested(selectedClip) }
                    ToolAction { wide: true; iconPath: root.iconOpen; text: "Open clip"; onClicked: if (selectedClip) backend.openClip(selectedClip.path) }
                    ToolAction { wide: true; iconPath: root.iconReveal; text: "Show in folder"; onClicked: if (selectedClip) backend.revealInExplorer(selectedClip.path) }
                    ToolAction { wide: true; iconPath: root.iconClipboard; text: "Copy clip"; onClicked: if (selectedClip) backend.copyClipToClipboard(selectedClip.path) }
                    ToolAction { wide: true; iconPath: root.iconDelete; text: "Delete clip"; danger: true; onClicked: requestDeleteClip(selectedClip) }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    Popup {
        id: deleteConfirm
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.min(page.width - 48, 460)
        x: Math.round((page.width - width) / 2)
        y: Math.round((page.height - height) / 2)
        padding: 16
        onOpened: deleteCancel.forceActiveFocus()
        onClosed: {
            if (page.gridMode)
                clipGrid.forceActiveFocus()
            else
                clipList.forceActiveFocus()
        }
        background: GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            border.color: root.recordRed
        }
        contentItem: ColumnLayout {
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                MonoIcon {
                    source: root.iconDelete
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    tint: root.recordRed
                    iconOpacity: 0.92
                }
                Text {
                    Layout.fillWidth: true
                    text: "Delete " + (page.pendingDeleteName.length > 0 ? page.pendingDeleteName : "clip") + "?"
                    color: root.textPrimary
                    font: root.titleFont
                    elide: Text.ElideRight
                }
            }

            Text {
                Layout.fillWidth: true
                text: "This removes the clip from disk. This cannot be undone."
                color: root.textSecondary
                font: root.bodyFont
                wrapMode: Text.Wrap
            }

            GlassPanel {
                Layout.fillWidth: true
                theme: root.uiTheme
                tone: "alt"
                implicitHeight: filePathText.implicitHeight + 24

                Text {
                    id: filePathText
                    anchors.fill: parent
                    anchors.margins: 12
                    text: page.pendingDeletePath
                    color: root.textSoft
                    font: root.monoFont
                    wrapMode: Text.WrapAnywhere
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }

                AppButton {
                    id: deleteCancel
                    theme: root.uiTheme
                    text: "Cancel"
                    onClicked: deleteConfirm.close()
                }

                AppButton {
                    theme: root.uiTheme
                    iconSource: root.iconDelete
                    text: "Delete clip"
                    danger: true
                    onClicked: {
                        backend.deleteClip(page.pendingDeletePath)
                        page.pendingDeletePath = ""
                        page.pendingDeleteName = ""
                        deleteConfirm.close()
                    }
                }
            }
        }
    }

    Menu {
        id: clipContextMenu
        width: 196
        modal: false
        padding: 6
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            border.color: root.borderStrong
        }

        ContextMenuItem {
            menuIcon: root.iconEdit
            text: "Trim and share"
            enabled: !!page.contextClip
            onTriggered: if (page.contextClip) page.editClipRequested(page.contextClip)
        }
        ContextMenuItem {
            menuIcon: root.iconOpen
            text: "Open clip"
            enabled: !!page.contextClip
            onTriggered: if (page.contextClip) backend.openClip(page.contextClip.path)
        }
        ContextMenuItem {
            menuIcon: root.iconReveal
            text: "Show in folder"
            enabled: !!page.contextClip
            onTriggered: if (page.contextClip) backend.revealInExplorer(page.contextClip.path)
        }
        ContextMenuItem {
            menuIcon: root.iconClipboard
            text: "Copy clip"
            enabled: !!page.contextClip
            onTriggered: if (page.contextClip) backend.copyClipToClipboard(page.contextClip.path)
        }
        ContextMenuSeparator {}
        ContextMenuItem {
            menuIcon: root.iconRefresh
            text: "Refresh"
            onTriggered: backend.requestRefresh()
        }
        ContextMenuItem {
            menuIcon: root.iconDelete
            text: "Delete clip"
            danger: true
            enabled: !!page.contextClip
            onTriggered: requestDeleteClip(page.contextClip)
        }
    }

    function isSelected(path) {
        return selectedClip && selectedClip.path === path
    }

    function applyFilter() {
        let all = backend.clipLibrary
        let query = searchField.textValue.toLowerCase()
        let game = gameFilter.currentText
        let result = []
        for (let i = 0; i < all.length; i++) {
            let clip = all[i]
            if (game && game !== "All Games" && clip.game !== game) continue
            if (query) {
                let name = (clip.name || "").toLowerCase()
                let g = (clip.game || "").toLowerCase()
                let date = (clip.date || "").toLowerCase()
                if (name.indexOf(query) < 0 && g.indexOf(query) < 0 && date.indexOf(query) < 0)
                    continue
            }
            result.push(clip)
        }
        filteredModel = result
        if (selectedClip) {
            let stillExists = false
            for (let i = 0; i < result.length; ++i) {
                if (result[i].path === selectedClip.path) {
                    selectedClip = result[i]
                    stillExists = true
                    break
                }
            }
            if (!stillExists)
                selectedClip = result.length > 0 ? result[0] : null
        } else if (result.length > 0) {
            selectedClip = result[0]
        }
    }

    function getGameFilters() {
        let games = ["All Games"]
        let seen = {}
        let all = backend.clipLibrary
        for (let i = 0; i < all.length; i++) {
            let g = all[i].game || ""
            if (g && !seen[g]) {
                seen[g] = true
                games.push(g)
            }
        }
        return games
    }

    function openClipContext(clip, px, py) {
        contextClip = clip
        clipContextMenu.x = Math.max(8, Math.min(px, page.width - clipContextMenu.width - 8))
        clipContextMenu.y = Math.max(8, Math.min(py, page.height - 220))
        clipContextMenu.open()
    }

    function hasActiveFilters() {
        return searchField.textValue.length > 0 || gameFilter.currentIndex > 0
    }

    function clearFilters() {
        searchField.textValue = ""
        gameFilter.currentIndex = 0
        applyFilter()
    }

    function requestDeleteClip(clip) {
        if (!clip)
            return
        selectedClip = clip
        pendingDeletePath = clip.path || ""
        pendingDeleteName = clip.name || clipFileName(pendingDeletePath)
        deleteConfirm.open()
    }

    function emptyClipsTitle() {
        return backend.clipLibrary.length === 0 ? "No clips yet" : "No clips match"
    }

    function emptyClipsText() {
        return backend.clipLibrary.length === 0
            ? "No clips saved yet. Save a replay with " + root.shortcutText() + " after a moment you want to keep."
            : "Try a different search, switch games, or clear the current filter."
    }

    function clipFileName(path) {
        let normalized = path.replace(/\\/g, "/")
        let idx = normalized.lastIndexOf("/")
        return idx >= 0 ? normalized.substring(idx + 1) : normalized
    }

    Component.onCompleted: {
        backend.requestRefresh()
        applyFilter()
    }
    onVisibleChanged: if (visible) { backend.requestRefresh(); applyFilter() }

    Connections {
        target: backend
        function onClipLibraryChanged() {
            gameFilter.model = getGameFilters()
            applyFilter()
        }
    }

    component HeaderText: Text {
        color: root.textMuted
        font: root.smallFont
        verticalAlignment: Text.AlignVCenter
    }

    component SearchField: GlassPanel {
        id: searchRoot

        property alias placeholder: placeholderText.text
        property alias textValue: field.text
        signal textChanged()

        theme: root.uiTheme
        tone: "raised"
        Layout.preferredHeight: 38
        border.color: field.activeFocus ? root.accent : root.border
        color: root.panelRaised

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 8
            MonoIcon { source: root.iconSearch; Layout.preferredWidth: 15; Layout.preferredHeight: 15; tint: root.textSoft; iconOpacity: 0.78 }
            TextField {
                id: field
                Layout.fillWidth: true
                color: root.textPrimary
                font: root.bodyFont
                selectByMouse: true
                placeholderText: ""
                background: Item {}
                onTextChanged: searchRoot.textChanged()
            }
        }

        Text {
            id: placeholderText
            visible: !field.text && !field.activeFocus
            text: ""
            color: root.textSoft
            font: root.bodyFont
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 30
        }
    }

    component FlatCombo: ComboBox {
        id: combo
        Layout.preferredHeight: 38
        font: root.smallFont

        background: GlassPanel {
            theme: root.uiTheme
            tone: "raised"
            border.color: combo.activeFocus ? root.accent : root.border
            color: root.panelRaised
        }

        contentItem: Text {
            text: combo.displayText
            color: root.textSecondary
            font: combo.font
            verticalAlignment: Text.AlignVCenter
            leftPadding: 10
            rightPadding: 22
            elide: Text.ElideRight
        }

        delegate: ItemDelegate {
            width: combo.width
            contentItem: Text {
                text: modelData
                color: root.textPrimary
                font: root.bodyFont
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: highlighted ? root.hoverBg : root.panelRaised }
        }

        popup: Popup {
            y: combo.height + 4
            width: combo.width
            padding: 4
            background: GlassPanel {
                theme: root.uiTheme
                tone: "raised"
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

    component SegmentedButton: AppButton {
        property bool active: false
        property string iconPath
        theme: root.uiTheme
        iconSource: iconPath
        accentColor: root.controlAccent(iconPath, false)
        primary: active
        Layout.preferredWidth: 62
        Layout.preferredHeight: 38
    }

    component ToolAction: AppButton {
        property string iconPath
        property bool wide: false
        theme: root.uiTheme
        iconSource: iconPath
        accentColor: root.controlAccent(iconPath, danger)
        Layout.preferredWidth: wide ? 286 : Math.max(72, implicitWidth)
        Layout.preferredHeight: 38
    }

    component TinyAction: AppButton {
        property string iconPath
        theme: root.uiTheme
        iconSource: iconPath
        accentColor: root.controlAccent(iconPath, danger)
        Layout.preferredWidth: compact ? 32 : Math.max(72, implicitWidth)
        Layout.minimumWidth: Layout.preferredWidth
        Layout.maximumWidth: Layout.preferredWidth
        Layout.preferredHeight: 28
    }

    component EmptyState: Column {
        property real maxWidth: 520
        width: maxWidth
        spacing: 12

        MonoIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            source: backend.clipLibrary.length === 0 ? root.iconSave : root.iconSearch
            width: 24
            height: 24
            tint: backend.clipLibrary.length === 0 ? root.accentGreen : root.accentBlue
            iconOpacity: 0.78
        }

        Text {
            width: parent.width
            text: emptyClipsTitle()
            color: root.textPrimary
            font: root.titleFont
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: emptyClipsText()
            color: root.textSoft
            font: root.bodyFont
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            AppButton {
                visible: backend.clipLibrary.length > 0 && hasActiveFilters()
                theme: root.uiTheme
                iconSource: root.iconRefresh
                text: "Clear filters"
                onClicked: clearFilters()
            }

            AppButton {
                visible: backend.clipLibrary.length === 0
                theme: root.uiTheme
                iconSource: root.iconFolder
                text: "Open folder"
                onClicked: backend.openOutputDir()
            }

            AppButton {
                theme: root.uiTheme
                iconSource: root.iconRefresh
                text: "Refresh"
                onClicked: backend.requestRefresh()
            }
        }
    }

    component InspectorBlock: GlassPanel {
        id: inspectorBlock

        property string title
        property var rows

        theme: root.uiTheme
        tone: "raised"
        Layout.fillWidth: true
        implicitHeight: inspectorColumn.implicitHeight + 24

        ColumnLayout {
            id: inspectorColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            Text { text: inspectorBlock.title; color: root.textMuted; font: root.smallFont }
            Repeater {
                model: inspectorBlock.rows
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: modelData.label; color: root.textSoft; font: root.smallFont }
                    Text { text: modelData.value; color: root.textSecondary; font: modelData.label === "File" ? root.monoFont : root.bodyFont; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                }
            }
        }
    }

    component ContextMenuItem: MenuItem {
        id: contextMenuItem
        property string menuIcon: ""
        property bool danger: false
        implicitWidth: 200
        implicitHeight: 36
        leftPadding: 12
        rightPadding: 12

        contentItem: RowLayout {
            spacing: 8

            MonoIcon {
                visible: contextMenuItem.menuIcon.length > 0
                source: contextMenuItem.menuIcon
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                tint: contextMenuItem.danger ? root.recordRed : root.textSecondary
                iconOpacity: contextMenuItem.enabled ? 0.82 : 0.35
            }

            Text {
                text: contextMenuItem.text
                color: !contextMenuItem.enabled ? root.textSoft : (contextMenuItem.danger ? root.recordRed : root.textSecondary)
                font: root.bodyFont
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        background: Rectangle {
            radius: 8
            color: contextMenuItem.highlighted ? root.hoverBg : "transparent"
            border.color: contextMenuItem.highlighted ? root.border : "transparent"
            border.width: 1
        }
    }

    component ContextMenuSeparator: MenuSeparator {
        topPadding: 6
        bottomPadding: 6

        contentItem: Rectangle {
            implicitWidth: 194
            implicitHeight: 1
            color: root.border
        }
    }
}
