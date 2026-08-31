pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/**
 * The Ctrl+K palette: symbols from the index, then the fixed command list.
 *
 * The two sections share one selection, so ↑/↓ walk straight from the last symbol into the first
 * action: `selection` indexes the two lists concatenated, split at completionList.count.
 */
Item {
    id: root

    anchors.fill: parent
    visible: SearchViewModel.paletteOpen

    property int selection: 0

    function close() {
        SearchViewModel.paletteOpen = false
    }

    function accept() {
        if (root.selection < completionList.count)
            SearchViewModel.activateCompletion(root.selection)
        else
            SearchViewModel.runAction(root.selection - completionList.count)
    }

    function move(delta) {
        const total = completionList.count + actionList.count
        if (total > 0)
            root.selection = (root.selection + delta + total) % total
    }

    onVisibleChanged: {
        if (visible) {
            root.selection = 0
            input.forceActiveFocus()
            input.selectAll()
        }
    }

    // Dismiss on a click outside the panel.
    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        TapHandler { onTapped: root.close() }
    }

    Rectangle {
        id: panel

        anchors.horizontalCenter: parent.horizontalCenter
        y: 120
        width: 660
        height: Math.min(parent.height - 240, content.implicitHeight)
        color: Theme.background
        border.width: 1
        border.color: Theme.accentLine

        StCornerMarks { color: Theme.edge; topRight: false; bottomLeft: false }

        // Clicks inside the panel must not reach the scrim behind it.
        TapHandler {}

        Column {
            id: content
            width: parent.width

            Rectangle {
                width: parent.width
                height: Theme.sized(54)
                color: "transparent"

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.paddingWide
                    anchors.rightMargin: Theme.paddingWide
                    spacing: Theme.padding

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "⌕"
                        color: Theme.accent
                        font.pixelSize: Theme.fontIntro
                    }

                    TextInput {
                        id: input
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 40
                        text: SearchViewModel.query
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontIntro
                        selectionColor: Theme.selection
                        selectedTextColor: Theme.text
                        clip: true

                        onTextChanged: {
                            SearchViewModel.query = text
                            root.selection = 0
                        }

                        Keys.onDownPressed: root.move(1)
                        Keys.onUpPressed: root.move(-1)
                        Keys.onEscapePressed: root.close()
                        Keys.onReturnPressed: root.accept()
                        Keys.onEnterPressed: root.accept()

                        Text {
                            anchors.fill: parent
                            visible: input.text.length === 0
                            text: qsTr("Search symbols, files or text…")
                            color: Theme.textFaint
                            font: input.font
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.line
                }
            }

            StSectionLabel {
                visible: completionList.count > 0
                leftPadding: Theme.paddingWide
                topPadding: Theme.spacing
                bottomPadding: Theme.spacingTight
                text: qsTr("Symbols")
                color: Theme.textFainter
            }

            ListView {
                id: completionList
                width: parent.width
                height: Math.min(contentHeight, Theme.sized(32) * 6)
                clip: true
                reuseItems: true
                interactive: contentHeight > height
                model: SearchViewModel.completions

                delegate: MatchRow {
                    required property int index
                    required property string matchName
                    required property string matchSubtext
                    required property string matchTypeName
                    required property var matchIndices

                    width: ListView.view.width
                    name: matchName
                    subtext: matchSubtext
                    typeName: matchTypeName
                    indices: matchIndices
                    current: root.selection === index
                    onActivated: SearchViewModel.activateCompletion(index)
                }
            }

            StSectionLabel {
                visible: actionList.count > 0
                leftPadding: Theme.paddingWide
                topPadding: Theme.spacing
                bottomPadding: Theme.spacingTight
                text: qsTr("Actions")
                color: Theme.textFainter
            }

            ListView {
                id: actionList
                width: parent.width
                height: contentHeight
                interactive: false
                model: SearchViewModel.actions

                delegate: Item {
                    id: actionRow
                    required property int index
                    required property string actionLabel
                    required property string actionGlyph
                    required property string actionShortcut

                    width: ListView.view.width
                    height: Theme.sized(32)

                    readonly property bool current: root.selection === completionList.count + actionRow.index

                    Rectangle {
                        anchors.fill: parent
                        color: actionRow.current ? Theme.selected
                             : (actionHover.hovered ? Theme.raised : "transparent")
                    }

                    Rectangle {
                        visible: actionRow.current
                        width: 2
                        height: parent.height
                        color: Theme.accent
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.paddingWide
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.padding

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: actionRow.actionGlyph
                            color: Theme.textFaint
                            font.pixelSize: Theme.fontSize
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: actionRow.actionLabel
                            color: actionRow.current ? Theme.text : Theme.textDim
                            font.family: Theme.bodyFamily
                            font.pixelSize: Theme.fontBody
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.paddingWide
                        anchors.verticalCenter: parent.verticalCenter
                        text: actionRow.actionShortcut
                        color: Theme.textFaint
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontTiny
                    }

                    HoverHandler { id: actionHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: SearchViewModel.runAction(actionRow.index) }
                }
            }

            Rectangle {
                width: parent.width
                height: Theme.sized(36)
                color: "transparent"

                Rectangle {
                    width: parent.width
                    height: 1
                    color: Theme.line
                }

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.paddingWide
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.paddingWide

                    Repeater {
                        model: [qsTr("↑↓ navigate"), qsTr("↵ open"), qsTr("esc close")]

                        delegate: Text {
                            required property string modelData
                            text: modelData
                            color: Theme.textFaint
                            font.family: Theme.bodyFamily
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                }
            }
        }
    }
}
