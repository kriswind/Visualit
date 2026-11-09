#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// Paints one row of the commit graph: the lane lines crossing the row and the commit node.
class CommitGraph : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<int> segments READ segments WRITE setSegments NOTIFY changed)
    Q_PROPERTY(int lane READ lane WRITE setLane NOTIFY changed)
    Q_PROPERTY(bool merge READ merge WRITE setMerge NOTIFY changed)
    Q_PROPERTY(bool head READ head WRITE setHead NOTIFY changed)
    Q_PROPERTY(qreal laneWidth READ laneWidth WRITE setLaneWidth NOTIFY changed)
    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY changed)
    Q_PROPERTY(QColor nodeFill READ nodeFill WRITE setNodeFill NOTIFY changed)

public:
    explicit CommitGraph(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QList<int> segments() const { return m_segments; }
    void setSegments(const QList<int> &segments);
    int lane() const { return m_lane; }
    void setLane(int lane);
    bool merge() const { return m_merge; }
    void setMerge(bool merge);
    bool head() const { return m_head; }
    void setHead(bool head);
    qreal laneWidth() const { return m_laneWidth; }
    void setLaneWidth(qreal width);
    QList<QColor> colors() const { return m_colors; }
    void setColors(const QList<QColor> &colors);
    QColor nodeFill() const { return m_nodeFill; }
    void setNodeFill(const QColor &color);

signals:
    void changed();

private:
    template<typename T>
    void assign(T &member, const T &value);
    QColor colorFor(int index) const;

    QList<int> m_segments;
    QList<QColor> m_colors;
    QColor m_nodeFill = Qt::white;
    qreal m_laneWidth = 14;
    int m_lane = 0;
    bool m_merge = false;
    bool m_head = false;
};
