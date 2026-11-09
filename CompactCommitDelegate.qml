pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

// Graph-only row. The node is a hit target that opens the commit; the rest of the row selects it.
Rectangle {
    id: root

    required property int index
    required property string shortSha
    required property string subject
    required property string author
    required property date date
    required property var refs
    required property int lane
    required property list<int> segments
    required property bool isMerge
    required property bool isHead

    readonly property bool current: ListView.isCurrentItem

    signal clicked()
    signal nodeClicked()

    height: Theme.compactRowHeight
    // Translucent so lanes and nodes stay visible through the selection.
    color: current ? Qt.tint(palette.base, Qt.alpha(palette.highlight, ListView.view.activeFocus ? 0.35 : 0.18))
                   : palette.base

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("%1, by %2, %3").arg(subject).arg(author).arg(Theme.shortDate(date))

    TapHandler {
        onTapped: root.clicked()
    }

    CommitGraph {
        id: graph
        x: Theme.spacing / 2
        width: root.width - x
        height: root.height
        segments: root.segments
        lane: root.lane
        merge: root.isMerge
        head: root.isHead
        laneWidth: Theme.compactLaneWidth
        colors: Theme.laneColors
        nodeFill: root.color
    }

    Item {
        id: node
        x: graph.x + root.lane * Theme.compactLaneWidth
        width: Theme.compactLaneWidth
        height: root.height

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Open %1 in browser").arg(root.shortSha)

        TapHandler {
            onTapped: root.nodeClicked()
        }
        HoverHandler {
            id: nodeHover
            cursorShape: Qt.PointingHandCursor
        }

        ToolTip.visible: nodeHover.hovered
        ToolTip.delay: 300
        ToolTip.text: {
            const names = root.refs.map(r => r.name).join(", ")
            const lines = [root.subject, `${root.shortSha} · ${root.author} · ${Theme.relativeTime(root.date)}`]
            if (names)
                lines.splice(1, 0, names)
            return lines.join("\n")
        }
    }
}
