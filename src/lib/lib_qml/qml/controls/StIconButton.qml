import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/** A square, bordered glyph button: the toolbar's back/forward/home, the graph's zoom controls. */
Button {
    id: root

    /** Rail buttons are borderless and fill on hover instead. */
    property bool borderless: false
    property bool active: false
    /** Overrides the resting glyph colour; hover and disabled still win. */
    property color iconColor: root.borderless ? Theme.iconMuted : Theme.textDim

    implicitWidth: root.borderless ? Theme.railButtonSize : Theme.iconButtonSize
    implicitHeight: implicitWidth
    hoverEnabled: true

    background: Rectangle {
        radius: Theme.radius
        color: root.active ? Theme.selected : (root.hovered && root.enabled ? Theme.raised : "transparent")
        border.width: root.borderless ? 0 : 1
        border.color: root.active ? Theme.accentLine : (root.hovered && root.enabled ? Theme.accent : Theme.lineStrong)
    }

    contentItem: Text {
        text: root.text
        font.family: Theme.bodyFamily
        font.pixelSize: Theme.fontSize
        color: !root.enabled ? Theme.line
             : root.active ? Theme.accentHi
             : root.hovered ? Theme.text
             : root.iconColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
