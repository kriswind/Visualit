#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

struct Commit
{
    QString sha;
    QStringList parents;
    QString author;
    QString email;
    QString subject;
    QDateTime date;
    // Each entry: { name, kind: "local" | "remote" | "tag", current: bool }
    QVariantList refs;
    bool isHead = false;

    // Graph layout, filled by CommitModel.
    int lane = 0;
    // Flat list of segments, 4 ints each: half (0 = top, 1 = bottom), fromLane, toLane, colorIndex.
    QList<int> segments;
};

class CommitModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by GitRepository")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int maxLanes READ maxLanes NOTIFY countChanged)

public:
    enum Role {
        ShaRole = Qt::UserRole + 1,
        ShortShaRole,
        SubjectRole,
        AuthorRole,
        EmailRole,
        DateRole,
        RefsRole,
        ParentsRole,
        LaneRole,
        SegmentsRole,
        IsMergeRole,
        IsHeadRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int maxLanes() const { return m_maxLanes; }
    const QList<Commit> &commits() const { return m_commits; }

    void setCommits(QList<Commit> commits);

    Q_INVOKABLE int indexOfSha(const QString &shaPrefix) const;
    Q_INVOKABLE QString shaAt(int row) const;
    // Case-insensitive search over subject, author, email, sha prefix and ref names.
    // Wraps around; returns -1 when nothing matches.
    Q_INVOKABLE int find(const QString &text, int from, bool forward) const;

signals:
    void countChanged();

private:
    static int layoutGraph(QList<Commit> &commits);

    QList<Commit> m_commits;
    QHash<QString, int> m_rowBySha;
    int m_maxLanes = 0;
};
