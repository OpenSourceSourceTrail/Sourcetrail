pragma ComponentBehavior: Bound
import QtQuick
import Sourcetrail

/** The navigation history, newest at the bottom, current entry marked. */
ListView {
    id: root

    clip: true
    // The history can grow long over a session; recycle rather than instantiating every row.
    reuseItems: true
    model: NavigationViewModel.history
    currentIndex: NavigationViewModel.currentIndex

    delegate: Item {
        id: row

        required property int index
        required property string historyName
        required property string historyTypeName
        required property bool historyIsCurrent

        width: ListView.view.width
        height: Theme.sized(28)

        Rectangle {
            anchors.fill: parent
            color: row.historyIsCurrent ? Theme.selected : (hover.hovered ? Theme.raised : "transparent")
        }

        Rectangle {
            visible: row.historyIsCurrent
            width: 2
            height: parent.height
            color: Theme.accent
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingWide
            anchors.right: kind.left
            anchors.rightMargin: Theme.spacing
            anchors.verticalCenter: parent.verticalCenter
            text: row.historyName
            color: row.historyIsCurrent ? Theme.accentHi : Theme.textDim
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontCode
            elide: Text.ElideMiddle
        }

        Text {
            id: kind
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingWide
            anchors.verticalCenter: parent.verticalCenter
            text: row.historyTypeName
            color: Theme.textFaint
            font.family: Theme.bodyFamily
            font.pixelSize: Theme.fontMicro
        }

        HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: NavigationViewModel.goToPosition(row.index) }
    }
}
