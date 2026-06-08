#include "mobile/GrimVaultController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QIcon>

#include <sodium.h>

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        return 1;
    }
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GrimLedger"));
    QCoreApplication::setOrganizationName(QStringLiteral("grimsec-labs"));

    GrimVaultController vault;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vault"), &vault);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
