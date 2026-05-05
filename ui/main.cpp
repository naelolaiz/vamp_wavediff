#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int
main(int argc, char* argv[])
{
  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("WaveDiff"));
  QGuiApplication::setOrganizationName(QStringLiteral("vamp_wavediff"));
  QGuiApplication::setApplicationVersion(QStringLiteral("0.2.0"));

  QQuickStyle::setStyle(QStringLiteral("Material"));

  QQmlApplicationEngine engine;
  QObject::connect(
    &engine,
    &QQmlApplicationEngine::objectCreated,
    &app,
    [](QObject* obj, const QUrl&) {
      if (!obj) {
        QCoreApplication::exit(-1);
      }
    },
    Qt::QueuedConnection);

  engine.loadFromModule(QStringLiteral("WaveDiff"), QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty()) {
    return -1;
  }

  return app.exec();
}
