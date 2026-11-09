#include "folderpicker.h"

#include <QGuiApplication>
#include <QRandomGenerator>
#include <QUrl>

#if VISUALIT_HAVE_DBUS
#  include <QDBusConnection>
#  include <QDBusConnectionInterface>
#  include <QDBusMessage>
#  include <QDBusPendingCallWatcher>
#  include <QDBusPendingReply>
#  include <QDBusObjectPath>
#endif

using namespace Qt::StringLiterals;

namespace {
constexpr auto PortalService = "org.freedesktop.portal.Desktop";
constexpr auto PortalPath = "/org/freedesktop/portal/desktop";
constexpr auto FileChooserInterface = "org.freedesktop.portal.FileChooser";
constexpr auto RequestInterface = "org.freedesktop.portal.Request";
} // namespace

FolderPicker::FolderPicker(QObject *parent)
    : QObject(parent)
{
}

void FolderPicker::open(const QString &title, const QString &currentFolder, QWindow *parent)
{
#if VISUALIT_HAVE_DBUS
    if (isBusy())
        return;

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !bus.interface()
        || !bus.interface()->isServiceRegistered(QString::fromLatin1(PortalService)).value()) {
        emit unavailable();
        return;
    }

    // Subscribing to the predicted request path before calling avoids missing a fast Response.
    const QString token = u"visualit%1"_s.arg(QRandomGenerator::global()->generate());
    QString sender = bus.baseService().mid(1);
    sender.replace(u'.', u'_');
    m_requestPath = u"/org/freedesktop/portal/desktop/request/%1/%2"_s.arg(sender, token);
    bus.connect(QString::fromLatin1(PortalService), m_requestPath, QString::fromLatin1(RequestInterface),
                u"Response"_s, this, SLOT(handleResponse(uint, QVariantMap)));
    emit busyChanged();

    QVariantMap options{
        { u"handle_token"_s, token },
        { u"directory"_s, true },
        { u"modal"_s, true },
        { u"accept_label"_s, tr("Open") },
    };
    if (!currentFolder.isEmpty()) {
        QByteArray folder = currentFolder.toUtf8();
        folder.append('\0');
        options.insert(u"current_folder"_s, folder);
    }

    // X11 windows can be identified to the portal; Wayland needs xdg-foreign, so it goes unparented.
    QString parentHandle;
    if (parent && QGuiApplication::platformName() == u"xcb"_s)
        parentHandle = u"x11:%1"_s.arg(parent->winId(), 0, 16);

    QDBusMessage call = QDBusMessage::createMethodCall(QString::fromLatin1(PortalService),
                                                       QString::fromLatin1(PortalPath),
                                                       QString::fromLatin1(FileChooserInterface),
                                                       u"OpenFile"_s);
    call << parentHandle << title << options;

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(call), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<QDBusObjectPath> reply = *w;
        if (reply.isError()) {
            finish();
            emit unavailable();
            return;
        }
        // Older portals ignore handle_token and return a different request path.
        const QString actual = reply.value().path();
        if (!m_requestPath.isEmpty() && actual != m_requestPath) {
            QDBusConnection bus = QDBusConnection::sessionBus();
            bus.disconnect(QString::fromLatin1(PortalService), m_requestPath,
                           QString::fromLatin1(RequestInterface), u"Response"_s, this,
                           SLOT(handleResponse(uint, QVariantMap)));
            m_requestPath = actual;
            bus.connect(QString::fromLatin1(PortalService), m_requestPath,
                        QString::fromLatin1(RequestInterface), u"Response"_s, this,
                        SLOT(handleResponse(uint, QVariantMap)));
        }
    });
#else
    Q_UNUSED(title)
    Q_UNUSED(currentFolder)
    Q_UNUSED(parent)
    emit unavailable();
#endif
}

void FolderPicker::handleResponse(uint response, const QVariantMap &results)
{
    finish();
    // 0 = success, 1 = cancelled by the user, 2 = ended some other way.
    if (response == 0) {
        const QStringList uris = results.value(u"uris"_s).toStringList();
        if (!uris.isEmpty()) {
            emit accepted(QUrl(uris.first()).toLocalFile());
            return;
        }
    }
    emit rejected();
}

void FolderPicker::finish()
{
#if VISUALIT_HAVE_DBUS
    if (m_requestPath.isEmpty())
        return;
    QDBusConnection::sessionBus().disconnect(QString::fromLatin1(PortalService), m_requestPath,
                                             QString::fromLatin1(RequestInterface), u"Response"_s,
                                             this, SLOT(handleResponse(uint, QVariantMap)));
    m_requestPath.clear();
    emit busyChanged();
#endif
}
