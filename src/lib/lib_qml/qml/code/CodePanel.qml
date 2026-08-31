pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sourcetrail

/**
 * The code panel: the reference bar, the Snippets / Full file switch, and the snippets themselves.
 *
 * Snippets arrive already flattened across files, each carrying the index of the file it came from,
 * so a file header is drawn wherever that index changes rather than by nesting a second view.
 */
Rectangle {
    id: root

    color: Theme.panelAlt

    Rectangle {
        width: 1
        height: parent.height
        color: Theme.line
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 1
        spacing: 0

        // "N references to X", with the previous/next stepper the design puts beside it.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.panelHeaderHeight
            color: Theme.panel
            visible: CodeViewModel.referenceCount > 0

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.padding
                anchors.rightMargin: Theme.spacingTight
                spacing: Theme.spacingTight

                Text {
                    Layout.fillWidth: true
                    text: qsTr("%1 references").arg(CodeViewModel.referenceCount)
                    color: Theme.textMuted
                    font.family: Theme.bodyFamily
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }

                Text {
                    visible: CodeViewModel.referenceCount > 0
                    text: qsTr("%1/%2").arg(CodeViewModel.referenceIndex).arg(CodeViewModel.referenceCount)
                    color: Theme.textFaint
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontTiny
                }

                StIconButton {
                    text: "↑"
                    onClicked: CodeViewModel.previousReference()
                }

                StIconButton {
                    text: "↓"
                    onClicked: CodeViewModel.nextReference()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.line
        }

        StSegmented {
            Layout.fillWidth: true
            Layout.margins: Theme.spacingTight
            visible: !CodeViewModel.empty
            model: [qsTr("Snippets"), qsTr("Full file")]
            currentIndex: CodeViewModel.listMode ? 0 : 1
            onActivated: function(index) { CodeViewModel.setListMode(index === 0) }
        }

        ListView {
            id: snippetList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: CodeViewModel.snippets
            spacing: Theme.spacing
            // Each delegate owns a text document that C++ decorates; recycling them would hand the
            // same document to a different row, so this view deliberately does not reuse items.
            reuseItems: false

            ScrollBar.vertical: ScrollBar {}

            delegate: Column {
                id: snippetDelegate

                required property int index
                required property string snippetTitle
                required property string snippetFooter
                required property string snippetCode
                required property int snippetStartLine
                required property string snippetFileName
                required property string snippetFilePath
                required property bool snippetFirstOfFile

                width: snippetList.width
                spacing: 0

                // A file header, drawn once per file rather than once per snippet.
                Rectangle {
                    visible: snippetDelegate.snippetFirstOfFile
                    width: snippetDelegate.width
                    height: visible ? Theme.panelHeaderHeight : 0
                    color: Theme.card

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: Theme.padding
                        anchors.rightMargin: Theme.padding
                        text: snippetDelegate.snippetFileName
                        color: Theme.text
                        font.family: Theme.bodyFamily
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideMiddle
                    }

                    TapHandler {
                        onSingleTapped: CodeViewModel.showFile(snippetDelegate.snippetFilePath, true)
                    }
                }

                Snippet {
                    width: snippetDelegate.width
                    row: snippetDelegate.index
                    title: snippetDelegate.snippetTitle
                    footer: snippetDelegate.snippetFooter
                    code: snippetDelegate.snippetCode
                    startLine: snippetDelegate.snippetStartLine
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingWide * 2
        horizontalAlignment: Text.AlignHCenter
        visible: CodeViewModel.empty
        text: qsTr("No code to show")
        color: Theme.textFaint
        font.family: Theme.bodyFamily
        font.pixelSize: Theme.fontCode
        wrapMode: Text.WordWrap
    }
}
