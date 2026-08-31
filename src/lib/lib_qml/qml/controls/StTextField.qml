import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/** The design's single input treatment: dark well, hairline border, accent border on focus. */
TextField {
    id: root

    property bool mono: false

    implicitHeight: Theme.controlHeight
    leftPadding: 10
    rightPadding: 10
    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectionColor: Theme.selection
    selectedTextColor: Theme.text
    font.family: root.mono ? Theme.monoFamily : Theme.bodyFamily
    font.pixelSize: root.mono ? Theme.fontCode : Theme.fontSize

    background: Rectangle {
        radius: Theme.radius
        color: Theme.chrome
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : Theme.lineStrong
    }
}
