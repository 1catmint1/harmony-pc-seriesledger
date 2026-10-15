#pragma once

#include "appmodel.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrlQuery>

#include <functional>

class InfluxClient final : public QObject
{
    Q_OBJECT

public:
    using Callback = std::function<void(bool success, const QJsonDocument &document, const QString &error)>;

    explicit InfluxClient(const ConnectionProfile &profile, QObject *parent = nullptr);

    const ConnectionProfile &profile() const { return m_profile; }

    void ping(const Callback &callback);
    void query(const QString &database, const QString &statement, const Callback &callback);
    void postQuery(const QString &database, const QString &statement, const Callback &callback);

private:
    QString baseUrl() const;
    void request(const QByteArray &method, const QString &path, const QByteArray &query,
                 const QByteArray &body, const Callback &callback);

    ConnectionProfile m_profile;
    QNetworkAccessManager m_network;
};

