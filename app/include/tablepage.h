#pragma once

#include <QJsonDocument>
#include <QWidget>

class QLabel;
class QTableWidget;

class TablePage final : public QWidget
{
    Q_OBJECT

public:
    explicit TablePage(const QString &title, QWidget *parent = nullptr);

    void setDocument(const QJsonDocument &document);
    void setError(const QString &error);
    void setLoading(bool loading);
    void invalidate();
    const QJsonDocument &document() const { return m_document; }

public slots:
    void exportCsv();
    void exportJson();

private:
    QString m_title;
    QLabel *m_status = nullptr;
    QTableWidget *m_table = nullptr;
    QJsonDocument m_document;
};

