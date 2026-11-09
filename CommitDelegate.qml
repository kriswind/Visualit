pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    required property int index
    required property string sha
    required property string shortSha
    required property string subject
    required property string author
    required property date date
    required property var refs
    required property int lane
    required property list<int> segments
    required property bool isMerge
    required property bool isHead

    property real graphWidth
    property real authorWidth
    property real dateWidth
    property real shaWidth

    readonly property bool current: ListView.isCurrentItem
    readonly property color textColor: current && ListView.view.activeFocus ? palette.highlightedText : palette.text
    readonly property color secondaryText: current && ListView.view.activeFocus ? palette.highlightedText
                                                                                : palette.placeholderText

    signal clicked()
    signal activated()

    height: Theme.rowHeight
    color: current ? (ListView.view.activeFocus ? palette.highlight : palette.midlight)
                   : index % 2 ? palette.alternateBase : palette.base

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("%1, by %2, %3").arg(subject).arg(author).arg(Theme.shortDate(date))

    TapHandler {
        onTapped: root.clicked()
        onDoubleTapped: root.activated()
    }

    CommitGraph {
        id: graph
        x: Theme.spacing / 2
        width: root.graphWidth
        height: root.height
        segments: root.segments
        lane: root.lane
        merge: root.isMerge
        head: root.isHead
        laneWidth: Theme.laneWidth
        colors: Theme.laneColors
        nodeFill: root.color
    }

    Row {
        id: messageCell
        x: graph.x + graph.width
        width: root.width - x - root.authorWidth - root.dateWidth - root.shaWidth - Theme.spacing / 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        // At most two chips per row so the subject stays readable; the rest collapse into "+N".
        Row {
            id: chips
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model: root.refs.slice(0, 2)
                RefChip {
                    required property var modelData
                    name: modelData.name
                    kind: modelData.kind
                    current: modelData.current
                    maximumWidth: Math.max(100, messageCell.width * 0.3)
                }
            }
            Label {
                visible: root.refs.length > 2
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("+%1").arg(root.refs.length - 2)
                font.pointSize: Theme.fontCaption
                color: root.secondaryText

                HoverHandler { id: moreHover }
                ToolTip.visible: moreHover.hovered
                ToolTip.text: root.refs.map(r => r.name).join("\n")
            }
        }

        Label {
            width: Math.max(0, messageCell.width - chips.width - messageCell.spacing - Theme.spacing)
            anchors.verticalCenter: parent.verticalCenter
            text: root.subject
            elide: Text.ElideRight
            color: root.textColor
            font.weight: root.isHead ? Font.Medium : Font.Normal
        }
    }

    Label {
        x: messageCell.x + messageCell.width
        width: root.authorWidth - Theme.spacing
        anchors.verticalCenter: parent.verticalCenter
        text: root.author
        elide: Text.ElideRight
        color: root.secondaryText
    }

    Label {
        x: messageCell.x + messageCell.width + root.authorWidth
        width: root.dateWidth - Theme.spacing
        anchors.verticalCenter: parent.verticalCenter
        text: Theme.shortDate(root.date)
        elide: Text.ElideRight
        color: root.secondaryText
    }

    Label {
        x: messageCell.x + messageCell.width + root.authorWidth + root.dateWidth
        width: root.shaWidth
        anchors.verticalCenter: parent.verticalCenter
        text: root.shortSha
        font.family: Theme.monoFamily
        color: root.secondaryText
    }
}
