pragma ComponentBehavior: Bound
import QtQuick
import Sourcetrail

/**
 * The four drafting `+` marks the design hangs just outside a panel's corners.
 *
 * Purely decorative, and deliberately outside the parent's bounds -- so whatever hosts one must not
 * clip. Anchored to the parent rather than sized, so it costs nothing to drop onto any container.
 */
Item {
    id: root

    /** Which corners to draw. Cards in the mockup often show only the diagonal pair. */
    property bool topLeft: true
    property bool topRight: true
    property bool bottomLeft: true
    property bool bottomRight: true

    property color color: Theme.cornerMark
    property int size: 11

    anchors.fill: parent
    z: 1

    component Mark: Text {
        text: "+"
        color: root.color
        font.pixelSize: root.size
        font.family: Theme.bodyFamily
    }

    Mark { visible: root.topLeft;     anchors.horizontalCenter: parent.left;  anchors.verticalCenter: parent.top }
    Mark { visible: root.topRight;    anchors.horizontalCenter: parent.right; anchors.verticalCenter: parent.top }
    Mark { visible: root.bottomLeft;  anchors.horizontalCenter: parent.left;  anchors.verticalCenter: parent.bottom }
    Mark { visible: root.bottomRight; anchors.horizontalCenter: parent.right; anchors.verticalCenter: parent.bottom }
}
