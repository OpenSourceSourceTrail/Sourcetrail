import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/**
 * The design has exactly two button weights: a solid accent fill for the one primary action on a
 * screen, and a hairline-bordered ghost for everything else. Both are square -- no radius anywhere.
 */
Button {
    id: root

    /** Solid accent fill. At most one per screen, per the design. */
    property bool primary: false
    /** Tighter padding for buttons that sit inside a toolbar or a table row. */
    property bool compact: false

    implicitHeight: root.compact ? Theme.controlHeight - 4 : Theme.controlHeight + 6
    leftPadding: root.compact ? 12 : 22
    rightPadding: root.compact ? 12 : 22
    hoverEnabled: true
    font.family: Theme.bodyFamily
    font.pixelSize: root.compact ? Theme.fontCode : Theme.fontMedium
    font.weight: root.primary ? Font.DemiBold : Font.Normal

    background: Rectangle {
        radius: Theme.radius
        color: root.primary
            ? (!root.enabled ? Theme.lineStrong : root.down || root.hovered ? Theme.accentHi : Theme.accent)
            : (root.down ? Theme.selected : root.hovered ? Theme.raised : "transparent")
        border.width: root.primary ? 0 : 1
        border.color: !root.enabled ? Theme.line : root.hovered ? Theme.accent : Theme.lineStrong
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.primary ? Theme.accentText : (root.enabled ? Theme.text : Theme.textFaint)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
