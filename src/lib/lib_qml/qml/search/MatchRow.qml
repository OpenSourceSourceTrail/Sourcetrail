pragma ComponentBehavior: Bound
import QtQuick
import Sourcetrail

/**
 * One symbol row in the palette: a kind badge, the name with matched characters highlighted, and
 * where it lives.
 */
Item {
    id: root

    required property string name
    required property string subtext
    required property string typeName
    required property var indices
    property bool current: false

    signal activated()

    implicitHeight: Theme.sized(32)

    Rectangle {
        anchors.fill: parent
        color: root.current ? Theme.selected : (hover.hovered ? Theme.raised : "transparent")
    }

    Rectangle {
        visible: root.current
        width: 2
        height: parent.height
        color: Theme.accent
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: Theme.paddingWide
        anchors.right: where.left
        anchors.rightMargin: Theme.padding
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.padding

        StChip {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.typeName.length > 0
            text: root.typeName
            accented: root.current
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            // The matched characters are marked up here rather than in C++: the view-model ships
            // the index list, and which colour "matched" is belongs to the theme.
            text: Theme.highlight(root.name, root.indices, Theme.accentHi)
            textFormat: Text.StyledText
            color: root.current ? Theme.text : Theme.textDim
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontBody
            elide: Text.ElideMiddle
        }
    }

    Text {
        id: where
        anchors.right: parent.right
        anchors.rightMargin: Theme.paddingWide
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(implicitWidth, root.width * 0.35)
        text: root.subtext
        color: Theme.textFaint
        font.family: Theme.bodyFamily
        font.pixelSize: Theme.fontSmall
        elide: Text.ElideLeft
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.activated() }
}
