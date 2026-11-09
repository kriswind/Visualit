import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: root

    required property GitRepository repository

    function show(text: string) {
        messageLabel.text = text
        clearTimer.restart()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing * 1.5
        anchors.rightMargin: Theme.spacing * 1.5
        spacing: Theme.spacing * 2

        Label {
            id: messageLabel
            Layout.fillWidth: true
            elide: Text.ElideRight
            font.pointSize: Theme.fontCaption
        }

        Label {
            text: root.repository.webUrl
                  ? root.repository.webUrl
                  : qsTr("⚠ No web remote — Enter cannot open commits")
            color: root.repository.webUrl ? palette.placeholderText : Theme.warning
            font.pointSize: Theme.fontCaption
            elide: Text.ElideMiddle
            Layout.maximumWidth: 360
        }

        Label {
            text: qsTr("↑↓ navigate · Enter open in browser · Ctrl+M compact · Ctrl+F search · F5 refresh")
            color: palette.placeholderText
            font.pointSize: Theme.fontCaption
        }
    }

    Timer {
        id: clearTimer
        interval: 6000
        onTriggered: messageLabel.text = ""
    }
}
