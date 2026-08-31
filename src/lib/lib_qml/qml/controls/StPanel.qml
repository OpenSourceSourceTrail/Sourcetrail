import QtQuick
import Sourcetrail

/** A bordered surface with an optional set of drafting corner marks. The design's basic container. */
Rectangle {
    id: root

    property bool marks: false
    property bool accented: false

    radius: Theme.radius
    color: Theme.panelAlt
    border.width: 1
    border.color: root.accented ? Theme.accentLine : Theme.line

    StCornerMarks {
        visible: root.marks
        color: root.accented ? Theme.edge : Theme.cornerMark
    }
}
