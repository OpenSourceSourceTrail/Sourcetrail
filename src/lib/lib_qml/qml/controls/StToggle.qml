import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/** The square sliding toggle from the Preferences rows. No radius, no animation curve tricks. */
Switch {
    id: root

    implicitWidth: 40
    implicitHeight: 22

    indicator: Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: root.checked ? Theme.selection : Theme.chrome
        border.width: 1
        border.color: root.checked ? Theme.accent : Theme.lineStrong

        Rectangle {
            width: 16
            height: 16
            radius: Theme.radius
            y: 2
            x: root.checked ? parent.width - width - 2 : 2
            color: root.checked ? Theme.accent : Theme.edge

            Behavior on x { NumberAnimation { duration: 90 } }
        }
    }

    contentItem: Item {}
}
