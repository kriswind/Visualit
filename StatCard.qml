import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root

    property string title
    property string value
    property string caption

    padding: Theme.spacing * 1.5

    Accessible.role: Accessible.StaticText
    Accessible.name: `${title}: ${value} ${caption}`

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        Label {
            text: root.title
            color: palette.placeholderText
            font.pointSize: Theme.fontCaption
        }
        Label {
            Layout.fillWidth: true
            text: root.value
            font.pointSize: Theme.fontDisplay
            font.weight: Font.Medium
            elide: Text.ElideRight
        }
        Label {
            Layout.fillWidth: true
            text: root.caption
            color: palette.placeholderText
            font.pointSize: Theme.fontCaption
            elide: Text.ElideRight
        }
    }
}
