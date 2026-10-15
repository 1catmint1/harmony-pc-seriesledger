#include "influxclient.h"
#include "jsonhelpers.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslError>
#include <QTimer>
#include <QPointer>
#include <memory>

namespace {
QByteArray queryParameters(const QString &database, const QString &statement)
{
    QByteArray encoded;
    if (!database.trimmed().isEmpty()) encoded = "db=" + QUrl::toPercentEncoding(database) + '&';
    return encoded + "q=" + QUrl::toPercentEncoding(statement) + "&epoch=ms";
}
}

InfluxClient::InfluxClient(const ConnectionProfile &profile, QObject *parent)
    : QObject(parent), m_profile(profile)
{
    m_network.setParent(this);
}

QString InfluxClient::baseUrl() const
{
    QString host = m_profile.host.trimmed();
    if (host.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)) {
        host.remove(0, 7);
    } else if (host.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        host.remove(0, 8);
    }
    const int slash = host.indexOf('/');
    if (slash >= 0) {
        host.truncate(slash);
    }
    const QString scheme = m_profile.useSsl ? QStringLiteral("https") : QStringLiteral("http");
    return QStringLiteral("%1://%2:%3").arg(scheme, host).arg(m_profile.port);
}

void InfluxClient::ping(const Callback &callback)
{
    request("GET", QStringLiteral("/ping"), QByteArray(), QByteArray(), callback);
}

void InfluxClient::query(const QString &database, const QString &statement, const Callback &callback)
{
    request("GET", QStringLiteral("/query"), queryParameters(database, statement), QByteArray(), callback);
}

void InfluxClient::postQuery(const QString &database, const QString &statement, const Callback &callback)
{
    request("POST", QStringLiteral("/query"), QByteArray(), queryParameters(database, statement), callback);
}

void InfluxClient::request(const QByteArray &method, const QString &path, const QByteArray &query,
                           const QByteArray &body, const Callback &callback)
{
    QUrl url(baseUrl() + path);
    url.setQuery(QString::fromLatin1(query), QUrl::StrictMode);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("InfluxDBStudio-Qt/1.0"));
    if (!m_profile.username.isEmpty()) {
        const QByteArray credentials = (m_profile.username + QStringLiteral(":") + m_profile.password).toUtf8().toBase64();
        request.setRawHeader("Authorization", "Basic " + credentials);
    }
    if (method == "POST") {
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          QStringLiteral("application/x-www-form-urlencoded"));
    }

    QNetworkReply *reply = nullptr;
    if (method == "POST") {
        reply = m_network.post(request, body);
    } else {
        reply = m_network.get(request);
    }

    if (m_profile.ignoreSslErrors) {
        connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
            reply->ignoreSslErrors();
        });
    }

    struct ResponseState { QByteArray payload; QString failure; };
    auto state = std::make_shared<ResponseState>();
    auto drain = [reply, state]() {
        if (!state->failure.isEmpty()) return;
        if (reply->bytesAvailable() > 8 * 1024 * 1024 - state->payload.size()) {
            state->failure = QStringLiteral("Response limit exceeded (8 MiB)"); reply->abort(); return;
        }
        state->payload += reply->readAll();
    };
    connect(reply, &QNetworkReply::readyRead, this, drain);
    QTimer::singleShot(15000, reply, [reply, state]() {
        if (!reply->isFinished()) { state->failure = QStringLiteral("Request timeout (15 s)"); reply->abort(); }
    });
    QPointer<QNetworkReply> guardedReply(reply);
    connect(reply, &QNetworkReply::finished, this, [reply, guardedReply, callback, path, state, drain]() {
        drain();
        const QByteArray payload = state->payload;
        const bool transportOk = reply->error() == QNetworkReply::NoError;
        QString error;
        QJsonDocument document;
        if (!payload.isEmpty()) {
            QJsonParseError parseError;
            document = QJsonDocument::fromJson(payload, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                error = QStringLiteral("InfluxDB returned invalid JSON: %1").arg(parseError.errorString());
            }
        }
        if (!state->failure.isEmpty()) {
            error = state->failure;
        } else if (!transportOk) {
            error = reply->errorString();
        } else if (!document.isNull()) {
            error = influxResponseError(document);
        } else if (path == QStringLiteral("/query")) {
            error = QStringLiteral("Empty InfluxDB query response");
        }
        if (callback) {
            callback(transportOk && error.isEmpty(), document, error);
        }
        if (guardedReply) guardedReply->deleteLater();
    });
}
