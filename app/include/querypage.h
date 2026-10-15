#pragma once

#include <QWidget>
#include <QPointer>

class InfluxClient;
class QPlainTextEdit;
class TablePage;

class QueryPage final : public QWidget
{
    Q_OBJECT

public:
    explicit QueryPage(InfluxClient *client, const QString &database, QWidget *parent = nullptr);

    QString database() const { return m_database; }
    void setDatabase(const QString &database);
    void setQuery(const QString &query);

public slots:
    void run();

private:
    QPointer<InfluxClient> m_client;
    quint64 m_requestRevision = 0;
    QString m_database;
    QPlainTextEdit *m_editor = nullptr;
    TablePage *m_result = nullptr;
};

