#pragma once

#include <QObject>
#include <QVariantMap>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

// Asks the desktop for a folder through the XDG desktop portal (org.freedesktop.portal.FileChooser),
// so the user gets their system's own file picker (e.g. the KDE/Dolphin dialog on Plasma).
// Emits unavailable() when no portal can be reached, so the caller can show its own dialog.
class FolderPicker : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit FolderPicker(QObject *parent = nullptr);

    bool isBusy() const { return !m_requestPath.isEmpty(); }

    Q_INVOKABLE void open(const QString &title, const QString &currentFolder, QWindow *parent = nullptr);

signals:
    void accepted(const QString &path);
    void rejected();
    void unavailable();
    void busyChanged();

private slots:
    void handleResponse(uint response, const QVariantMap &results);

private:
    void finish();

    QString m_requestPath;
};
