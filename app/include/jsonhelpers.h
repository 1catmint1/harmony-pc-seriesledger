#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QSet>

struct TableData
{
    QStringList columns;
    QVector<QStringList> rows;
    QString error;
};

inline QString jsonValueToText(const QJsonValue &value);

inline QString influxResponseError(const QJsonDocument &document)
{
    if (!document.isObject()) return QStringLiteral("Invalid InfluxDB response envelope");
    const QJsonObject root = document.object();
    if (!root.value(QStringLiteral("error")).toString().isEmpty())
        return root.value(QStringLiteral("error")).toString();
    if (!root.value(QStringLiteral("results")).isArray()) return QStringLiteral("Missing InfluxDB results array");
    const QJsonArray results = root.value(QStringLiteral("results")).toArray();
    for (int i = 0; i < results.size(); ++i) {
        if (!results.at(i).isObject()) return QStringLiteral("Invalid statement result");
        const QJsonObject result = results.at(i).toObject();
        const QString error = result.value(QStringLiteral("error")).toString();
        if (!error.isEmpty()) return QStringLiteral("statement %1: %2")
            .arg(result.value(QStringLiteral("statement_id")).toInt(i)).arg(error);
    }
    return QString();
}

inline TableData tableDataFromInflux(const QJsonDocument &document)
{
    TableData result;
    result.error = influxResponseError(document);
    if (!result.error.isEmpty()) return result;
    const QJsonObject root = document.object();
    const QJsonArray results = root.value(QStringLiteral("results")).toArray();
    QHash<QString, int> indexes;
    auto addColumn = [&](const QString &name) {
        if (indexes.contains(name)) return;
        if (result.columns.size() >= 256) { result.error = QStringLiteral("Result column limit exceeded (256)"); return; }
        indexes.insert(name, result.columns.size()); result.columns << name;
        for (QStringList &row : result.rows) row << QString();
    };
    for (int statement = 0; statement < results.size(); ++statement) {
        const QJsonValue resultValue = results.at(statement);
        const QJsonObject resultObject = resultValue.toObject();
        if (resultObject.contains(QStringLiteral("series")) && !resultObject.value(QStringLiteral("series")).isArray()) {
            result.error = QStringLiteral("Invalid series array"); result.columns.clear(); result.rows.clear(); return result;
        }
        const QJsonArray series = resultObject.value(QStringLiteral("series")).toArray();
        for (const QJsonValue &seriesValue : series) {
            if (!seriesValue.isObject()) { result.error = QStringLiteral("Invalid series object"); result.columns.clear(); result.rows.clear(); return result; }
            const QJsonObject seriesObject = seriesValue.toObject();
            if (!seriesObject.value(QStringLiteral("columns")).isArray() || !seriesObject.value(QStringLiteral("values")).isArray()) {
                result.error = QStringLiteral("Invalid series columns or values"); result.columns.clear(); result.rows.clear(); return result;
            }
            const QString seriesName = seriesObject.value(QStringLiteral("name")).toString();
            const QJsonArray seriesColumns = seriesObject.value(QStringLiteral("columns")).toArray();
            const QJsonObject tags = seriesObject.value(QStringLiteral("tags")).toObject();
            addColumn(QStringLiteral("statement_id")); addColumn(QStringLiteral("measurement"));
            for (auto tag = tags.begin(); tag != tags.end(); ++tag) addColumn(QStringLiteral("tag.") + tag.key());
            QSet<QString> seen;
            for (const QJsonValue &column : seriesColumns) {
                if (!column.isString()) { result.error = QStringLiteral("Invalid result column name"); break; }
                const QString key = QStringLiteral("field.") + column.toString();
                if (seen.contains(key)) { result.error = QStringLiteral("Duplicate result column"); break; }
                seen.insert(key); addColumn(key);
            }
            if (!result.error.isEmpty()) { result.columns.clear(); result.rows.clear(); return result; }
            const QJsonArray values = seriesObject.value(QStringLiteral("values")).toArray();
            for (const QJsonValue &rowValue : values) {
                const QJsonArray row = rowValue.toArray();
                if (!rowValue.isArray() || row.size() != seriesColumns.size() || result.rows.size() >= 10000) {
                    result.error = QStringLiteral("Invalid row shape or result row limit exceeded (10000)");
                    result.columns.clear(); result.rows.clear(); return result;
                }
                QStringList fields; for (int i = 0; i < result.columns.size(); ++i) fields << QString();
                fields[indexes.value(QStringLiteral("statement_id"))] = QString::number(resultObject.value(QStringLiteral("statement_id")).toInt(statement));
                fields[indexes.value(QStringLiteral("measurement"))] = seriesName;
                for (auto tag = tags.begin(); tag != tags.end(); ++tag) fields[indexes.value(QStringLiteral("tag.") + tag.key())] = jsonValueToText(tag.value());
                for (int i = 0; i < row.size(); ++i) fields[indexes.value(QStringLiteral("field.") + seriesColumns.at(i).toString())] = jsonValueToText(row.at(i));
                result.rows.push_back(fields);
            }
        }
    }
    return result;
}

inline QString jsonValueToText(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined()) {
        return QString();
    }
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 17);
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
}
