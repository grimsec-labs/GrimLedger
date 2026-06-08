#include "GrimVaultController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QIcon>
#include <QLoggingCategory>

#include <sodium.h>

int main(int argc, char* argv[]) {
    // sodium_init() must succeed before any crypto runs. Log loudly on failure so
    // the cause is visible in Logcat instead of a silent process exit.
    if (sodium_init() < 0) {
        qCritical("GrimLedger: sodium_init() failed; aborting startup.");
        return 1;
    }
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GrimLedger"));
    QCoreApplication::setOrganizationName(QStringLiteral("grimsec-labs"));

    GrimVaultController vault;
    QQmlApplicationEngine engine;

    // Surface QML object-creation failures (missing modules/plugins, import errors)
    // with a clear diagnostic rather than an empty-rootObjects() silent exit.
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl& url) {
            qCritical() << "GrimLedger: QML object creation failed for" << url;
        },
        Qt::QueuedConnection);

    engine.rootContext()->setContextProperty(QStringLiteral("vault"), &vault);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qCritical("GrimLedger: QML root failed to load (qrc:/qml/main.qml); aborting.");
        return 1;
    }
    return app.exec();
}
