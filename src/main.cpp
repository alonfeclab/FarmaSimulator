#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Simulación Farmacia"));
    app.setOrganizationName(QStringLiteral("FarmaciaSim"));
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/FarmaciaSim/app_icon.ico")));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("FarmaciaSim", "Main");

    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0)))
        window->setIcon(app.windowIcon());

    return app.exec();
}
