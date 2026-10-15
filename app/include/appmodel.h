#pragma once

#include <QJsonObject>
#include <QString>

struct ConnectionProfile
{
    int id = 0;
    QString name = QStringLiteral("Local InfluxDB");
    QString host = QStringLiteral("127.0.0.1");
    int port = 8086;
    QString database;
    QString username;
    QString password;
    bool useSsl = false;
    bool ignoreSslErrors = false;

    QString baseUrl() const
    {
        const QString scheme = useSsl ? QStringLiteral("https") : QStringLiteral("http");
        QString normalizedHost = host.trimmed();
        if (normalizedHost.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)) {
            normalizedHost.remove(0, 7);
        } else if (normalizedHost.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            normalizedHost.remove(0, 8);
        }
        const int slash = normalizedHost.indexOf('/');
        if (slash >= 0) normalizedHost.truncate(slash);
        return QStringLiteral("%1://%2:%3").arg(scheme, normalizedHost).arg(port);
    }

    QJsonObject toJson() const
    {
        QJsonObject value;
        value.insert(QStringLiteral("id"), id);
        value.insert(QStringLiteral("name"), name);
        value.insert(QStringLiteral("host"), host);
        value.insert(QStringLiteral("port"), port);
        value.insert(QStringLiteral("database"), database);
        value.insert(QStringLiteral("username"), username);
        // Session password is deliberately excluded from saved/exported profiles.
        value.insert(QStringLiteral("useSsl"), useSsl);
        value.insert(QStringLiteral("ignoreSslErrors"), ignoreSslErrors);
        return value;
    }

    static ConnectionProfile fromJson(const QJsonObject &value)
    {
        ConnectionProfile profile;
        profile.id = value.value(QStringLiteral("id")).toInt();
        profile.name = value.value(QStringLiteral("name")).toString(profile.name);
        profile.host = value.value(QStringLiteral("host")).toString(profile.host);
        profile.port = value.value(QStringLiteral("port")).toInt(profile.port);
        profile.database = value.value(QStringLiteral("database")).toString();
        profile.username = value.value(QStringLiteral("username")).toString();
        profile.password = value.value(QStringLiteral("password")).toString();
        profile.useSsl = value.value(QStringLiteral("useSsl")).toBool();
        profile.ignoreSslErrors = value.value(QStringLiteral("ignoreSslErrors")).toBool();
        return profile;
    }
};

enum class NodeType
{
    Connection,
    Database,
    Measurement
};
