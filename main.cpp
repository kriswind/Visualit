#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(u"visualit"_s);
    QGuiApplication::setOrganizationName(u"visualit"_s);
    QGuiApplication::setApplicationDisplayName(u"Visualit"_s);
    QGuiApplication::setApplicationVersion(u"0.1.0"_s);
    // Wayland compositors take the icon from the desktop entry matching this id;
    // X11 and other platforms use the window icon.
    QGuiApplication::setDesktopFileName(u"visualit"_s);
    QGuiApplication::setWindowIcon(QIcon(u":/icons/visualit.svg"_s));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Git branch visualization dashboard"_s);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(u"repository"_s,
                                 u"Path to a git repository (defaults to the current directory)."_s,
                                 u"[repository]"_s);
    const QCommandLineOption webUrlOption(
        u"web-url"_s,
        u"Base web URL used for commit links, e.g. https://github.com/owner/repo. "
        u"Overrides the URL derived from the git remote."_s,
        u"url"_s);
    parser.addOption(webUrlOption);
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    const QString path = QDir(positional.isEmpty() ? QDir::currentPath() : positional.first())
                             .absolutePath();

    // An explicit style from the environment or -style still wins.
    if (QQuickStyle::name().isEmpty())
        QQuickStyle::setStyle(u"Fusion"_s);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        { u"initialPath"_s, path },
        { u"webUrlOverride"_s, parser.value(webUrlOption) },
    });
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("VisualitUi", "Main");

    return app.exec();
}
