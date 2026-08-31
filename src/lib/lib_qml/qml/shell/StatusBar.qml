pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Sourcetrail

/** The bar along the bottom: index state on the left, counts and hints on the right. */
Rectangle {
    id: root

    property string paletteShortcutLabel: "⌘K"

    signal errorsRequested()

    implicitHeight: Theme.statusHeight
    color: Theme.panel

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.line
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingWide
        anchors.rightMargin: Theme.spacingWide
        spacing: Theme.spacingWide

        // The pulsing dot the design uses to say "work is happening".
        Rectangle {
            visible: StatusViewModel.indexing || StatusViewModel.busy
            Layout.preferredWidth: 7
            Layout.preferredHeight: 7
            color: Theme.accent

            SequentialAnimation on opacity {
                running: StatusViewModel.indexing || StatusViewModel.busy
                loops: Animation.Infinite
                NumberAnimation { from: 0.35; to: 1.0; duration: 700 }
                NumberAnimation { from: 1.0; to: 0.35; duration: 700 }
            }
        }

        Text {
            text: StatusViewModel.message
            color: StatusViewModel.isError ? Theme.warn : Theme.textMuted
            font.family: Theme.bodyFamily
            font.pixelSize: Theme.fontCode
            elide: Text.ElideRight
            Layout.maximumWidth: 520
        }

        Rectangle {
            visible: StatusViewModel.indexing
            Layout.preferredWidth: 180
            Layout.preferredHeight: 4
            color: Theme.line

            Rectangle {
                width: parent.width * Math.max(0, Math.min(100, StatusViewModel.indexingPercent)) / 100
                height: parent.height
                color: Theme.accent
            }
        }

        Item { Layout.fillWidth: true }

        Text {
            visible: StatusViewModel.ideStatus.length > 0
            text: StatusViewModel.ideStatus
            color: Theme.textFaint
            font.family: Theme.bodyFamily
            font.pixelSize: Theme.fontCode
        }

        Text {
            visible: StatusViewModel.errorCount > 0
            text: qsTr("%n problem(s)", "", StatusViewModel.errorCount)
            color: Theme.warn
            font.family: Theme.bodyFamily
            font.pixelSize: Theme.fontCode

            TapHandler { onTapped: root.errorsRequested() }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
        }

        Text {
            text: qsTr("%1 for commands").arg(root.paletteShortcutLabel)
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontSmall
        }
    }
}
