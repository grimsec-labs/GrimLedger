#include "security/BreachCheck.h"
#include "utils/AppSettings.h"

#include <QCryptographicHash>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace BreachCheck {

bool isEnabled() {
    return AppSettings::hibpCheckEnabled();
}

void setEnabled(bool enabled) {
    AppSettings::setHibpCheckEnabled(enabled);
}

Result checkPassword(const QString& password) {
    Result result;
    if (!isEnabled()) {
        result.error = QStringLiteral("Breach check is disabled in settings.");
        return result;
    }
    if (password.isEmpty()) {
        result.error = QStringLiteral("Password is empty.");
        return result;
    }

    const QByteArray sha1 = QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();
    const QString prefix = QString::fromLatin1(sha1.left(5));
    const QString suffix = QString::fromLatin1(sha1.mid(5));

    QNetworkAccessManager manager;
    const QUrl url(QStringLiteral("https://api.pwnedpasswords.com/range/") + prefix);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("GrimLedger/1.0"));
    // GL-SEC-011: request response padding so a network observer cannot infer the
    // queried hash-prefix bucket size from the response length. Padded entries carry
    // a count of 0 and are ignored below.
    request.setRawHeader("Add-Padding", "true");
    request.setTransferTimeout(8000);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(10000);

    QNetworkReply* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start();
    loop.exec();

    if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
        result.error = QStringLiteral("Could not reach breach database.");
        reply->deleteLater();
        return result;
    }

    const QString body = QString::fromUtf8(reply->readAll());
    reply->deleteLater();

    const QStringList lines = body.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QStringList parts = line.trimmed().split(QLatin1Char(':'));
        if (parts.size() != 2) {
            continue;
        }
        if (parts[0].compare(suffix, Qt::CaseInsensitive) == 0) {
            const int count = parts[1].toInt();
            result.ok = true;
            result.breached = count > 0;  // padded entries report a count of 0
            result.count = count;
            return result;
        }
    }

    result.ok = true;
    result.breached = false;
    return result;
}

} // namespace BreachCheck
