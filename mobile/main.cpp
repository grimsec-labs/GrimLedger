#include "GrimVaultController.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtQml/qqml.h>

#include <sodium.h>

namespace {

void registerQmlTypes() {
    qmlRegisterSingletonType(
        QUrl(QStringLiteral("qrc:/qml/Theme/Theme.qml")),
        "Theme", 1, 0, "Theme");

    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimButton.qml")), "controls", 1, 0, "GrimButton");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimTextField.qml")), "controls", 1, 0, "GrimTextField");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimTextArea.qml")), "controls", 1, 0, "GrimTextArea");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimListItem.qml")), "controls", 1, 0, "GrimListItem");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimTabButton.qml")), "controls", 1, 0, "GrimTabButton");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimSectionLabel.qml")), "controls", 1, 0, "GrimSectionLabel");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qml/controls/GrimCheckBox.qml")), "controls", 1, 0, "GrimCheckBox");
}

} // namespace

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        qCritical("GrimLedger: sodium_init() failed; aborting startup.");
        return 1;
    }

    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GrimLedger"));
    QCoreApplication::setOrganizationName(QStringLiteral("grimsec-labs"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    registerQmlTypes();

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
