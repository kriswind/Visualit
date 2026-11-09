import QtQuick
import QtQuick.Controls

// Local branches are filled, remote branches outlined, tags amber with a "◆" marker, and the
// checked-out branch marked "⦿", so ref kinds stay distinguishable without relying on colour.
Rectangle {
    id: root

    required property string name
    required property string kind
    required property bool current
    property real maximumWidth: 220

    readonly property bool isTag: kind === "tag"
    readonly property bool isRemote: kind === "remote"

    implicitWidth: Math.min(label.implicitWidth + 10, maximumWidth)
    implicitHeight: label.implicitHeight + 4
    radius: height / 2
    color: isRemote ? "transparent" : isTag ? Theme.tagRef : Theme.localRef
    border.width: isRemote ? 1 : current ? 1.5 : 0
    border.color: isRemote ? Theme.remoteRefText : Theme.localRefText

    Accessible.role: Accessible.StaticText
    Accessible.name: isTag ? qsTr("tag %1").arg(name)
                           : isRemote ? qsTr("remote branch %1").arg(name)
                                      : current ? qsTr("checked-out branch %1").arg(name)
                                                : qsTr("branch %1").arg(name)

    Label {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, root.maximumWidth - 10)
        elide: Text.ElideMiddle
        text: (root.isTag ? "◆ " : root.current ? "⦿ " : "") + root.name
        font.pointSize: Theme.fontCaption
        font.weight: root.current ? Font.Medium : Font.Normal
        color: root.isRemote ? Theme.remoteRefText : root.isTag ? Theme.tagRefText : Theme.localRefText
    }
}
