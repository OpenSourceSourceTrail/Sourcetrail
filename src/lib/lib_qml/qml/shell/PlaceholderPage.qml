import QtQuick
import Sourcetrail

/** Stands in for a context panel page whose view-model has not landed yet. */
Item {
    id: root

    property string message: ""

    Text {
        anchors.fill: parent
        anchors.margins: Theme.paddingWide
        text: root.message
        color: Theme.textFaint
        font.family: Theme.bodyFamily
        font.pixelSize: Theme.fontCode
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignTop
    }
}
