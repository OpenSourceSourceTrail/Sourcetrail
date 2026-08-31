pragma ComponentBehavior: Bound
import QtQuick
import Sourcetrail

/**
 * The joined single-choice strip: Graph/Hierarchy/Files, Light/Dark/System, Snippets/Full file.
 * One hairline box, segments divided by internal borders rather than gaps.
 */
Row {
    id: root

    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    spacing: 0

    Repeater {
        model: root.model

        delegate: Rectangle {
            id: segment
            required property var modelData
            required property int index

            readonly property bool current: root.currentIndex === segment.index

            implicitWidth: text.implicitWidth + 22
            implicitHeight: 26
            color: segment.current ? Theme.selected : (hover.hovered ? Theme.raised : "transparent")
            border.width: 1
            border.color: Theme.lineStrong
            // Collapse the shared edge so the strip reads as one box, not a row of boxes.
            Component.onCompleted: if (segment.index > 0) segment.anchors.leftMargin = -1

            Text {
                id: text
                anchors.centerIn: parent
                text: segment.modelData
                color: segment.current ? Theme.text : Theme.textMuted
                font.family: Theme.bodyFamily
                font.pixelSize: Theme.fontCode
            }

            HoverHandler { id: hover }
            TapHandler { onTapped: root.activated(segment.index) }
        }
    }
}
