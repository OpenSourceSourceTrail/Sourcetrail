pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/** The thin menu strip across the top of the workspace. */
Rectangle {
    id: root

    signal openProjectRequested()
    signal newProjectRequested()
    signal refreshRequested()
    signal preferencesRequested()
    signal legendRequested()

    implicitHeight: Theme.menuHeight
    color: Theme.panel

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.line
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingWide
        spacing: 0

        component MenuButton: Item {
            id: entry
            required property string label
            property alias menu: popup

            width: text.implicitWidth + 24
            height: root.height

            Text {
                id: text
                anchors.centerIn: parent
                text: entry.label
                color: hover.hovered || popup.opened ? Theme.text : Theme.textDim
                font.family: Theme.bodyFamily
                font.pixelSize: Theme.fontSize
            }

            HoverHandler { id: hover }
            TapHandler { onTapped: popup.opened ? popup.close() : popup.open() }

            Menu {
                id: popup
                y: entry.height
            }
        }

        MenuButton {
            label: qsTr("File")
            menu.contentData: [
                MenuItem { text: qsTr("New Project…"); onTriggered: root.newProjectRequested() },
                MenuItem { text: qsTr("Open Project…"); onTriggered: root.openProjectRequested() },
                MenuSeparator {},
                MenuItem { text: qsTr("Preferences…"); onTriggered: root.preferencesRequested() },
                MenuSeparator {},
                MenuItem { text: qsTr("Exit"); onTriggered: AppShell.quit() }
            ]
        }

        MenuButton {
            label: qsTr("Edit")
            menu.contentData: [
                MenuItem {
                    text: qsTr("Back")
                    enabled: NavigationViewModel.canGoBack
                    onTriggered: NavigationViewModel.goBack()
                },
                MenuItem {
                    text: qsTr("Forward")
                    enabled: NavigationViewModel.canGoForward
                    onTriggered: NavigationViewModel.goForward()
                }
            ]
        }

        MenuButton {
            label: qsTr("View")
            menu.contentData: [
                MenuItem { text: qsTr("Overview"); onTriggered: GraphViewModel.showOverview() },
                MenuItem { text: qsTr("Graph Legend"); onTriggered: root.legendRequested() }
            ]
        }

        MenuButton {
            label: qsTr("Project")
            menu.contentData: [
                MenuItem {
                    text: qsTr("Refresh Index")
                    enabled: !AppShell.indexing
                    onTriggered: root.refreshRequested()
                },
                MenuItem {
                    text: qsTr("Reindex Everything")
                    enabled: !AppShell.indexing
                    onTriggered: AppShell.refresh(true)
                }
            ]
        }

        MenuButton {
            label: qsTr("Help")
            menu.contentData: [
                MenuItem { text: qsTr("Graph Legend"); onTriggered: root.legendRequested() }
            ]
        }
    }
}
