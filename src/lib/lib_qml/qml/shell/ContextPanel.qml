pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Sourcetrail

/**
 * The 268px panel beside the rail. Its content is whichever rail entry is selected.
 *
 * Files, Bookmarks and Errors are placeholders until their view-models land in later phases;
 * History is live now, because NavigationViewModel already carries it.
 */
Rectangle {
    id: root

    property int currentPage: 0
    property string projectName: ""

    implicitWidth: Theme.panelWidth
    color: Theme.panelAlt

    readonly property var pageTitles: [
        qsTr("Overview"), qsTr("Files"), qsTr("Bookmarks"), qsTr("History"), qsTr("Errors")
    ]

    Rectangle {
        anchors.right: parent.right
        height: parent.height
        width: 1
        color: Theme.line
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.panelHeaderHeight
            color: "transparent"

            StSectionLabel {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingWide
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - Theme.spacingWide * 2
                elide: Text.ElideRight
                text: root.projectName.length > 0
                      ? root.pageTitles[root.currentPage] + " · " + root.projectName
                      : root.pageTitles[root.currentPage]
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.line
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentPage

            OverviewPage {}
            PlaceholderPage { message: qsTr("The file tree arrives with the files view-model.") }
            PlaceholderPage { message: qsTr("Bookmarks arrive with the bookmark view-model.") }
            HistoryPage {}
            PlaceholderPage { message: qsTr("Errors are listed in the dock below.") }
        }
    }
}
