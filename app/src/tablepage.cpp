#include "tablepage.h"

#include "jsonhelpers.h"

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

QString csvEscape(const QString &value)
{
    QString escaped = value;
    escaped.replace('"', QStringLiteral("\"\""));
    if (escaped.contains(',') || escaped.contains('"') || escaped.contains('\n') || escaped.contains('\r')) {
        escaped = QStringLiteral("\"") + escaped + QStringLiteral("\"");
    }
    return escaped;
}

}

TablePage::TablePage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title)
{
    auto *layout = new QVBoxLayout(this);
    auto *toolbar = new QHBoxLayout;
    m_status = new QLabel(title, this);
    m_status->setObjectName(QStringLiteral("pageStatus"));
    auto *csvButton = new QPushButton(tr("导出 CSV"), this);
    auto *jsonButton = new QPushButton(tr("导出 JSON"), this);
    connect(csvButton, &QPushButton::clicked, this, &TablePage::exportCsv);
    connect(jsonButton, &QPushButton::clicked, this, &TablePage::exportJson);
    toolbar->addWidget(m_status, 1);
    toolbar->addWidget(csvButton);
    toolbar->addWidget(jsonButton);
    layout->addLayout(toolbar);

    m_table = new QTableWidget(this);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table, 1);
}

void TablePage::setLoading(bool loading)
{
    if (loading) { m_document = QJsonDocument(); m_table->setRowCount(0); m_table->setColumnCount(0); }
    m_status->setText(loading ? tr("正在请求 InfluxDB…") : m_title);
    m_table->setEnabled(!loading);
}

void TablePage::invalidate()
{
    m_document = QJsonDocument();
    m_table->setRowCount(0); m_table->setColumnCount(0); m_table->setEnabled(true);
    m_status->setText(tr("尚未执行当前查询"));
}

void TablePage::setError(const QString &error)
{
    m_document = QJsonDocument();
    m_table->setEnabled(true);
    m_status->setText(tr("请求失败：%1").arg(error));
    m_table->clearContents();
    m_table->setRowCount(0);
    m_table->setColumnCount(0);
}

void TablePage::setDocument(const QJsonDocument &document)
{
    m_document = document;
    const TableData data = tableDataFromInflux(document);
    if (!data.error.isEmpty()) { setError(data.error); return; }
    m_table->setEnabled(true);
    m_table->setSortingEnabled(false);
    m_table->clear();
    m_table->setColumnCount(data.columns.size());
    m_table->setHorizontalHeaderLabels(data.columns);
    m_table->setRowCount(data.rows.size());
    for (int row = 0; row < data.rows.size(); ++row) {
        const QStringList &fields = data.rows.at(row);
        for (int column = 0; column < fields.size(); ++column) {
            m_table->setItem(row, column, new QTableWidgetItem(fields.at(column)));
        }
    }
    m_table->setSortingEnabled(true);
    m_status->setText(tr("%1 · %2 行").arg(m_title).arg(data.rows.size()));
}

void TablePage::exportCsv()
{
    if (!m_table || m_table->columnCount() == 0) {
        QMessageBox::information(this, tr("导出"), tr("当前没有可导出的结果。"));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("导出 CSV"),
                                                       QStringLiteral("influx-result.csv"),
                                                       tr("CSV 文件 (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"), file.errorString());
        return;
    }
    QTextStream stream(&file);
    for (int column = 0; column < m_table->columnCount(); ++column) {
        if (column) stream << ',';
        stream << csvEscape(m_table->horizontalHeaderItem(column)->text());
    }
    stream << '\n';
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int column = 0; column < m_table->columnCount(); ++column) {
            if (column) stream << ',';
            const QTableWidgetItem *item = m_table->item(row, column);
            stream << csvEscape(item ? item->text() : QString());
        }
        stream << '\n';
    }
}

void TablePage::exportJson()
{
    if (m_document.isNull()) { QMessageBox::information(this, tr("导出"), tr("没有可导出的成功查询结果。")); return; }
    const QString path = QFileDialog::getSaveFileName(this, tr("导出 JSON"),
                                                       QStringLiteral("influx-result.json"),
                                                       tr("JSON 文件 (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"), file.errorString());
        return;
    }
    file.write(m_document.toJson(QJsonDocument::Indented));
}
