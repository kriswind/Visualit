#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

struct Branch
{
    QString name;     // short name, e.g. "main" or "origin/main"
    QString refName;  // full ref, e.g. "refs/heads/main"
    QString sha;
    QString subject;
    QDateTime date;
    bool isRemote = false;
    bool isCurrent = false;
};

class BranchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by GitRepository")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        RefNameRole,
        ShaRole,
        SubjectRole,
        DateRole,
        IsRemoteRole,
        IsCurrentRole,
        SectionRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setBranches(QList<Branch> branches);
    const QList<Branch> &branches() const { return m_branches; }

    Q_INVOKABLE int indexOfRef(const QString &refName) const;
    Q_INVOKABLE QString refNameAt(int row) const;

signals:
    void countChanged();

private:
    QList<Branch> m_branches;
};
