#include "commitgraph.h"

#include <QPainter>
#include <QPainterPath>

CommitGraph::CommitGraph(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    connect(this, &CommitGraph::changed, this, [this] { update(); });
}

template<typename T>
void CommitGraph::assign(T &member, const T &value)
{
    if (member == value)
        return;
    member = value;
    emit changed();
}

void CommitGraph::setSegments(const QList<int> &segments) { assign(m_segments, segments); }
void CommitGraph::setLane(int lane) { assign(m_lane, lane); }
void CommitGraph::setMerge(bool merge) { assign(m_merge, merge); }
void CommitGraph::setHead(bool head) { assign(m_head, head); }
void CommitGraph::setLaneWidth(qreal width) { assign(m_laneWidth, width); }
void CommitGraph::setColors(const QList<QColor> &colors) { assign(m_colors, colors); }
void CommitGraph::setNodeFill(const QColor &color) { assign(m_nodeFill, color); }

QColor CommitGraph::colorFor(int index) const
{
    if (m_colors.isEmpty())
        return Qt::gray;
    return m_colors.at(index % m_colors.size());
}

void CommitGraph::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    const qreal h = height();
    const qreal mid = h / 2;
    const auto laneX = [this](int lane) { return m_laneWidth * lane + m_laneWidth / 2; };

    QPen pen;
    pen.setWidthF(2.0);
    pen.setCapStyle(Qt::RoundCap);
    painter->setBrush(Qt::NoBrush);

    for (qsizetype k = 0; k + 3 < m_segments.size(); k += 4) {
        const bool top = m_segments.at(k) == 0;
        const int from = m_segments.at(k + 1);
        const int to = m_segments.at(k + 2);
        const qreal y0 = top ? 0 : mid;
        const qreal y1 = top ? mid : h;
        // Extend half-rows by a pixel so adjacent rows join without hairline gaps.
        const qreal y0e = top ? y0 - 1 : y0;
        const qreal y1e = top ? y1 : y1 + 1;

        QPainterPath path;
        path.moveTo(laneX(from), y0e);
        if (from == to) {
            path.lineTo(laneX(to), y1e);
        } else {
            const qreal cy = (y0 + y1) / 2;
            path.cubicTo(laneX(from), cy, laneX(to), cy, laneX(to), y1e);
        }
        pen.setColor(colorFor(m_segments.at(k + 3)));
        painter->setPen(pen);
        painter->drawPath(path);
    }

    const QPointF centre(laneX(m_lane), mid);
    const QColor color = colorFor(m_lane);
    const qreal radius = std::min(m_head ? 5.5 : 4.5, h * 0.28);

    if (m_head) {
        const qreal ring = radius + std::min(3.0, h * 0.18);
        painter->setPen(QPen(color, 2.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(centre, ring, ring);
    }
    if (m_merge) {
        painter->setPen(QPen(color, 2.5));
        painter->setBrush(m_nodeFill);
    } else {
        painter->setPen(QPen(m_nodeFill, 1.5));
        painter->setBrush(color);
    }
    painter->drawEllipse(centre, radius, radius);
}
