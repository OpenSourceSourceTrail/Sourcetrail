import QtQuick
import Sourcetrail

/**
 * A small bordered token: the search box's active-symbol chips, node-kind badges (CLASS, FUNC),
 * the error dock's severity filters, the project cards' language tags.
 */
Rectangle {
    id: root

    property alias text: label.text
    /** Shows a `✕` the caller can act on. */
    property bool removable: false
    property bool accented: true
    property bool mono: false
    signal removed()
    signal clicked()

    implicitWidth: row.implicitWidth + 16
    implicitHeight: row.implicitHeight + 6
    radius: Theme.radius
    color: root.accented ? Theme.chip : "transparent"
    border.width: 1
    border.color: root.accented ? Theme.accentLine : Theme.lineStrong

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6

        Text {
            id: label

            // Deriving tracking from font.pixelSize would read the same grouped property object
            // this binding writes, which Qt reports as a binding loop. Carry the size separately.
            readonly property int size: root.mono ? Theme.fontSmall : Theme.fontMicro

            color: root.accented ? Theme.accentHi : Theme.textMuted
            font.family: root.mono ? Theme.monoFamily : Theme.displayFamily
            font.pixelSize: label.size
            font.capitalization: root.mono ? Font.MixedCase : Font.AllUppercase
            font.letterSpacing: root.mono ? 0 : Theme.tracking(Theme.trackBadge, label.size)
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: root.removable
            text: "✕"
            color: closeArea.containsMouse ? Theme.text : Theme.textFaint
            font.pixelSize: Theme.fontMicro
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                id: closeArea
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                onClicked: root.removed()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        // Sits below the close glyph's own area so removing never also selects.
        z: -1
        onClicked: root.clicked()
    }
}
