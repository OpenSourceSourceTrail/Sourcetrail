pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/** The 52px rail down the left edge. Each entry swaps what the context panel shows. */
Rectangle {
    id: root

    // Kept in sync with ContextPanel's pages by index; see AppFrame.
    readonly property int pageOverview: 0
    readonly property int pageFiles: 1
    readonly property int pageBookmarks: 2
    readonly property int pageHistory: 3
    readonly property int pageErrors: 4

    property int currentPage: pageOverview

    signal legendRequested()

    implicitWidth: Theme.railWidth
    color: Theme.panel

    Rectangle {
        anchors.right: parent.right
        height: parent.height
        width: 1
        color: Theme.line
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        spacing: Theme.spacingTight

        Repeater {
            model: [
                { glyph: "◈", tip: qsTr("Overview"), page: root.pageOverview },
                { glyph: "▤", tip: qsTr("Files"), page: root.pageFiles },
                { glyph: "★", tip: qsTr("Bookmarks"), page: root.pageBookmarks },
                { glyph: "↺", tip: qsTr("History"), page: root.pageHistory },
                { glyph: "▲", tip: qsTr("Errors"), page: root.pageErrors }
            ]

            delegate: StIconButton {
                id: railButton
                required property var modelData

                borderless: true
                text: railButton.modelData.glyph
                active: root.currentPage === railButton.modelData.page
                // The errors entry keeps its warning colour whether or not it is the current page.
                iconColor: railButton.modelData.page === root.pageErrors && StatusViewModel.errorCount > 0
                           ? Theme.warn : Theme.iconMuted
                ToolTip.text: railButton.modelData.tip
                ToolTip.visible: hovered
                ToolTip.delay: 400
                onClicked: root.currentPage = railButton.modelData.page
            }
        }
    }

    StIconButton {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        borderless: true
        text: "?"
        ToolTip.text: qsTr("Graph legend")
        ToolTip.visible: hovered
        ToolTip.delay: 400
        onClicked: root.legendRequested()
    }
}
