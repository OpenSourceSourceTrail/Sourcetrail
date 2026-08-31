pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Sourcetrail

/**
 * The main toolbar: navigation, the search field, the trail toggle, the errors chip, settings.
 *
 * The search field here is a target rather than an input -- clicking it opens the command palette,
 * which is where the real search lives. That is what the design specifies, and it keeps one code
 * path for symbol lookup instead of two.
 */
Rectangle {
    id: root

    property bool trailOpen: false
    property string paletteShortcutLabel: "⌘K"

    signal paletteRequested()
    signal trailToggled()
    signal errorsRequested()
    signal preferencesRequested()

    implicitHeight: Theme.toolbarHeight
    color: Theme.background

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.line
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.padding
        anchors.rightMargin: Theme.padding
        spacing: Theme.spacing

        RowLayout {
            spacing: 2

            StIconButton {
                text: "←"
                enabled: NavigationViewModel.canGoBack
                onClicked: NavigationViewModel.goBack()
            }

            StIconButton {
                text: "→"
                enabled: NavigationViewModel.canGoForward
                onClicked: NavigationViewModel.goForward()
            }
        }

        StIconButton {
            text: "⌂"
            onClicked: GraphViewModel.showOverview()
        }

        Rectangle {
            id: searchTarget

            Layout.fillWidth: true
            Layout.maximumWidth: 640
            Layout.preferredHeight: Theme.controlHeight
            color: Theme.chrome
            border.width: 1
            border.color: searchHover.hovered ? Theme.accent : Theme.lineStrong

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.padding
                anchors.rightMargin: Theme.spacing
                spacing: 10

                Text {
                    text: "⌕"
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSize
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Search symbols, files or text…")
                    color: Theme.textFaint
                    font.family: Theme.bodyFamily
                    font.pixelSize: Theme.fontSize
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.preferredWidth: shortcut.implicitWidth + 10
                    Layout.preferredHeight: shortcut.implicitHeight + 2
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.lineStrong

                    Text {
                        id: shortcut
                        anchors.centerIn: parent
                        text: root.paletteShortcutLabel
                        color: Theme.textFaint
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontMicro
                    }
                }
            }

            HoverHandler { id: searchHover; cursorShape: Qt.IBeamCursor }
            TapHandler { onTapped: root.paletteRequested() }
        }

        Item { Layout.fillWidth: true }

        StButton {
            compact: true
            text: qsTr("⤳ Custom trail")
            onClicked: root.trailToggled()
        }

        // Only shown when the index actually has problems, per the design.
        Rectangle {
            visible: StatusViewModel.errorCount > 0
            Layout.preferredWidth: errorLabel.implicitWidth + 24
            Layout.preferredHeight: Theme.controlHeight
            color: Theme.warnBg
            border.width: 1
            border.color: errorHover.hovered ? Theme.warn : Theme.warnLine

            Text {
                id: errorLabel
                anchors.centerIn: parent
                text: qsTr("▲ %n error(s)", "", StatusViewModel.errorCount)
                color: Theme.warn
                font.family: Theme.bodyFamily
                font.pixelSize: Theme.fontSize
            }

            HoverHandler { id: errorHover }
            TapHandler { onTapped: root.errorsRequested() }
        }

        StIconButton {
            Layout.preferredWidth: Theme.controlHeight
            Layout.preferredHeight: Theme.controlHeight
            text: "⚙"
            onClicked: root.preferencesRequested()
        }
    }
}
