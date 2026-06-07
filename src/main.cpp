#include "App.h"
#include "utils/Theme.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QStyleFactory>
#include <sodium.h>

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        return 1;
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("GrimLedger"));
    QApplication::setOrganizationName(QStringLiteral("GrimLedger"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icon.png")));

    QFont defaultFont(QStringLiteral("Segoe UI"), 10);
    app.setFont(defaultFont);

    // Fusion renders QSS reliably on Windows (unlike the native Windows style).
    if (QStyleFactory::keys().contains(QStringLiteral("Fusion"))) {
        app.setStyle(QStringLiteral("Fusion"));
    }
    Theme::apply(app, Theme::savedAccent());

    App grimApp;
    grimApp.start();

    return app.exec();
}
