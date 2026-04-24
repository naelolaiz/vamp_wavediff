#include "Backend.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int
main(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("WaveDiff"));
  QGuiApplication::setOrganizationName(QStringLiteral("vamp_wavediff"));
  QGuiApplication::setApplicationVersion(QStringLiteral("0.2.0"));

  QQuickStyle::setStyle(QStringLiteral("Material"));

  QQmlApplicationEngine engine;
  Backend backend;
  engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);

  const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
  QObject::connect(
    &engine,
    &QQmlApplicationEngine::objectCreated,
    &app,
    [url](QObject* obj, const QUrl& objUrl) {
      if (!obj && url == objUrl) {
        QCoreApplication::exit(-1);
      }
    },
    Qt::QueuedConnection);

  engine.load(url);
  if (engine.rootObjects().isEmpty()) {
    return -1;
  }

  return app.exec();
}
