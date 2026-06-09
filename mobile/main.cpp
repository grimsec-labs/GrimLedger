#include "GrimVaultController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QIcon>
#include <QLoggingCategory>

#include <sodium.h>

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        qCritical("GrimLedger: sodium_init() failed; aborting startup.");
        return 1;
    }
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GrimLedger"));
    QCoreApplication::setOrganizationName(QStringLiteral("grimsec-labs"));

    GrimVaultController vault;
    QQmlApplicationEngine engine;

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
