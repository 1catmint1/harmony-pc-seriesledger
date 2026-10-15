#include "querypage.h"

#include "influxclient.h"
#include "tablepage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QVBoxLayout>

QueryPage::QueryPage(InfluxClient *client, const QString &database, QWidget *parent)
    : QWidget(parent), m_client(client), m_database(database)
{
    auto *layout = new QVBoxLayout(this);
    auto *bar = new QHBoxLayout;
    auto *databaseLabel = new QLabel(this);
    databaseLabel->setObjectName(QStringLiteral("queryDatabase"));
    databaseLabel->setText(tr("数据库：%1").arg(database.isEmpty() ? tr("未指定") : database));
    bar->addWidget(databaseLabel, 1);
    auto *runButton = new QPushButton(tr("执行查询"), this);
    runButton->setDefault(true);
    connect(runButton, &QPushButton::clicked, this, &QueryPage::run);
    bar->addWidget(runButton);
    layout->addLayout(bar);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    auto *editorPanel = new QWidget(splitter);
    auto *editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    m_editor = new QPlainTextEdit(editorPanel);
    m_editor->setPlaceholderText(tr("输入 InfluxQL，例如：SELECT * FROM \"cpu\" LIMIT 100"));
    m_editor->setPlainText(database.isEmpty()
                               ? QStringLiteral("SHOW DATABASES")
                               : QStringLiteral("SHOW MEASUREMENTS"));
    m_editor->setTabChangesFocus(false);
    editorLayout->addWidget(m_editor);
    splitter->addWidget(editorPanel);

    m_result = new TablePage(tr("查询结果"), splitter);
    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        ++m_requestRevision; m_result->invalidate();
    });
    splitter->addWidget(m_result);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    layout->addWidget(splitter, 1);

    auto *shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), this);
    connect(shortcut, &QShortcut::activated, this, &QueryPage::run);
}

void QueryPage::setDatabase(const QString &database)
{
    ++m_requestRevision;
    m_database = database;
    m_result->invalidate();
    if (auto *label = findChild<QLabel *>(QStringLiteral("queryDatabase")))
        label->setText(tr("数据库：%1").arg(database.isEmpty() ? tr("未指定") : database));
}

void QueryPage::setQuery(const QString &query)
{
    ++m_requestRevision;
    m_editor->setPlainText(query);
    m_result->invalidate();
}

void QueryPage::run()
{
    if (!m_client) {
        m_result->setError(tr("没有可用的 InfluxDB 连接。"));
        return;
    }
    const QString query = m_editor->toPlainText().trimmed();
    if (query.isEmpty()) {
        m_result->setError(tr("查询语句不能为空。"));
        return;
    }
    m_result->setLoading(true);
    const quint64 revision = ++m_requestRevision;
    const QString database = m_database;
    QPointer<QueryPage> owner(this);
    m_client->query(database, query, [owner, revision, database, query](bool success, const QJsonDocument &document, const QString &error) {
        if (!owner || owner->m_requestRevision != revision || owner->m_database != database || owner->m_editor->toPlainText().trimmed() != query) return;
        if (success) {
            owner->m_result->setDocument(document);
        } else {
            owner->m_result->setError(error);
        }
    });
}

