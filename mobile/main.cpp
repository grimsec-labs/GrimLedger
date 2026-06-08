#include "GrimVaultController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QIcon>
#include <QDebug>

#include <QQuickStyle>

#include <sodium.h>

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        qCritical() << "sodium_init failed";
        return 1;
    }

    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GrimLedger"));
    QCoreApplication::setOrganizationName(QStringLiteral("grimsec-labs"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("grimsec-labs.github.io"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    GrimVaultController vault;
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    engine.rootContext()->setContextProperty(QStringLiteral("vault"), &vault);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [](QObject* obj, const QUrl& url) {
            if (!obj) {
                qCritical() << "Failed to load QML object:" << url;
            }
        },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "No QML root objects loaded.";
        return 1;
    }

#if defined(Q_OS_ANDROID)
    // Register JNI bridge after QML/UI startup so Qt Android activity is ready.
    extern void AndroidJniBridge_registerController(GrimVaultController* controller);
    AndroidJniBridge_registerController(&vault);
#endif

    return app.exec();
}
