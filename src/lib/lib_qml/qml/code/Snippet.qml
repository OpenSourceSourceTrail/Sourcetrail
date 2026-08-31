pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Sourcetrail

/**
 * One run of lines from a file: a title row, a line-number gutter and the code itself.
 *
 * The TextArea is read-only and exists for its text document, not for editing. Syntax colouring and
 * source-location backgrounds are both applied to that document from C++ -- see
 * CodeViewModel::decorate -- so nothing here formats anything; it only hands the document over and
 * reports where a click landed.
 */
Column {
    id: snippet

    required property int row
    required property string title
    required property string footer
    required property string code
    required property int startLine

    readonly property int lineCount: code === "" ? 0 : code.split("\n").length

    width: parent ? parent.width : 0
    spacing: 0

    Text {
        visible: snippet.title !== ""
        width: snippet.width - Theme.padding * 2
        x: Theme.padding
        topPadding: Theme.spacingTight
        bottomPadding: Theme.spacingTight
        text: snippet.title
        color: Theme.textFaint
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontTiny
        elide: Text.ElideMiddle
    }

    Row {
        width: snippet.width
        spacing: 0

        // The gutter is a plain column of numbers: a painted widget bought nothing here, and this
        // stays aligned with the text because both use the same mono face and pixel size.
        Column {
            id: gutter
            width: Theme.gutterWidth

            Repeater {
                model: snippet.lineCount

                delegate: Text {
                    required property int index
                    width: gutter.width - Theme.spacingTight
                    horizontalAlignment: Text.AlignRight
                    text: snippet.startLine + index
                    color: Theme.gutter
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontCode
                    lineHeight: codeArea.lineHeight
                    lineHeightMode: Text.ProportionalHeight
                }
            }
        }

        TextArea {
            id: codeArea

            readonly property real lineHeight: 1.4

            width: snippet.width - gutter.width
            padding: 0
            leftPadding: Theme.spacingTight
            readOnly: true
            selectByMouse: true
            wrapMode: TextArea.NoWrap
            text: snippet.code
            color: Theme.textDim
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontCode
            // QtRendering keeps glyph positions consistent with the C++ side's character offsets.
            renderType: Text.QtRendering
            background: null

            // Decorated exactly once, when the document exists. Doing it again from onTextChanged
            // would recurse without end: decorate() writes character formats, editing the document
            // is itself a text change, and the handler would call straight back into it. The list
            // does not recycle delegates, so one pass per delegate is also all that is needed.
            Component.onCompleted: CodeViewModel.decorate(codeArea.textDocument, snippet.row)

            TapHandler {
                onSingleTapped: function(point) {
                    CodeViewModel.activateLocationAt(snippet.row, codeArea.positionAt(point.position.x, point.position.y))
                }
            }
        }
    }

    Text {
        visible: snippet.footer !== ""
        x: Theme.padding
        topPadding: Theme.spacingTight
        bottomPadding: Theme.spacingTight
        text: snippet.footer
        color: Theme.textFaint
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontTiny
    }
}
