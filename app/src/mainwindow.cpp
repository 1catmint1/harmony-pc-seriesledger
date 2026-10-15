#include "mainwindow.h"
#include "release_notices.h"
#include <QDialogButtonBox>
#include <QPlainTextEdit>

#include "connectiondialog.h"
#include "influxclient.h"
#include "jsonhelpers.h"
#include "querypage.h"
#include "tablepage.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <climits>

namespace {

QString textFromTableRow(const QStringList &columns, const QStringList &row, const QString &columnName)
{
    const int index = columns.indexOf(QStringLiteral("field.") + columnName);
    return index >= 0 && index < row.size() ? row.at(index) : QString();
}

QString quotedIdentifier(const QString &value)
{
    QString escaped = value;
    escaped.replace('"', QStringLiteral("\"\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

QString quotedLiteral(const QString &value)
{
    QString escaped = value;
    escaped.replace('\'', QStringLiteral("''"));
    return QStringLiteral("'") + escaped + QStringLiteral("'");
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SeriesLedger · InfluxDB 1.x 查询"));
    resize(1440, 900);
    setDockNestingEnabled(true);

    createActions();
    createMenus();

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    m_tree = new QTreeWidget(splitter);
    m_tree->setHeaderLabel(tr("连接与资源"));
    m_tree->setMinimumWidth(280);
    m_tree->setAlternatingRowColors(true);
    connect(m_tree, &QTreeWidget::itemExpanded, this, &MainWindow::treeExpanded);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::treeDoubleClicked);

    m_tabs = new QTabWidget(splitter);
    m_tabs->setTabsClosable(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *page = m_tabs->widget(index); m_tabs->removeTab(index);
        if (page) page->deleteLater();
    });

    auto *welcome = new QLabel(tr("SeriesLedger\n\n左侧添加或选择一个 InfluxDB 1.x 连接。\n双击数据库/measurement 可快速打开查询。\nCtrl+Enter 执行当前查询。\n\n基于 CymaticLabs 和社区 Qt/HarmonyOS 适配。"), m_tabs);
    welcome->setAlignment(Qt::AlignCenter);
    welcome->setObjectName(QStringLiteral("welcome"));
    m_tabs->addTab(welcome, tr("欢迎"));

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);
    statusBar()->showMessage(tr("就绪 · Qt Widgets / OpenHarmony QPA"));

    loadSettings();
    rebuildTree();
    if (m_profiles.isEmpty()) {
        QTimer::singleShot(0, this, &MainWindow::manageConnections);
    }
}

void MainWindow::createActions()
{
    m_manageConnections = new QAction(tr("管理连接…"), this);
    m_manageConnections->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(m_manageConnections, &QAction::triggered, this, &MainWindow::manageConnections);

    m_importSettings = new QAction(tr("导入连接配置…"), this);
    connect(m_importSettings, &QAction::triggered, this, &MainWindow::importSettings);
    m_exportSettings = new QAction(tr("导出连接配置…"), this);
    connect(m_exportSettings, &QAction::triggered, this, &MainWindow::exportSettings);

    m_newQuery = new QAction(tr("新建查询"), this);
    m_newQuery->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
    connect(m_newQuery, &QAction::triggered, this, &MainWindow::newQuery);
    m_runQuery = new QAction(tr("执行当前查询"), this);
    m_runQuery->setShortcut(QKeySequence(QStringLiteral("Ctrl+Return")));
    connect(m_runQuery, &QAction::triggered, this, &MainWindow::runCurrentQuery);
    m_refresh = new QAction(tr("刷新资源树"), this);
    m_refresh->setShortcut(QKeySequence::Refresh);
    connect(m_refresh, &QAction::triggered, this, &MainWindow::refreshSelected);
    m_disconnect = new QAction(tr("断开连接"), this);
    connect(m_disconnect, &QAction::triggered, this, &MainWindow::disconnectSelected);

    m_createDatabase = new QAction(tr("创建数据库…"), this);
    connect(m_createDatabase, &QAction::triggered, this, &MainWindow::createDatabase);
    m_dropSelected = new QAction(tr("删除选中资源…"), this);
    connect(m_dropSelected, &QAction::triggered, this, &MainWindow::dropSelected);
    m_dropSeries = new QAction(tr("删除 Series…"), this);
    connect(m_dropSeries, &QAction::triggered, this, &MainWindow::dropSeries);
    m_showSeries = new QAction(tr("Show Series"), this);
    connect(m_showSeries, &QAction::triggered, this, &MainWindow::showSeries);
    m_showTagKeys = new QAction(tr("Show Tag Keys"), this);
    connect(m_showTagKeys, &QAction::triggered, this, &MainWindow::showTagKeys);
    m_showTagValues = new QAction(tr("Show Tag Values…"), this);
    connect(m_showTagValues, &QAction::triggered, this, &MainWindow::showTagValues);
    m_showFieldKeys = new QAction(tr("Show Field Keys"), this);
    connect(m_showFieldKeys, &QAction::triggered, this, &MainWindow::showFieldKeys);
    m_retentionPolicies = new QAction(tr("Retention Policies"), this);
    connect(m_retentionPolicies, &QAction::triggered, this, &MainWindow::showRetentionPolicies);
    m_createRetention = new QAction(tr("创建 Retention Policy…"), this);
    connect(m_createRetention, &QAction::triggered, this, &MainWindow::createRetentionPolicy);
    m_alterRetention = new QAction(tr("修改 Retention Policy…"), this);
    connect(m_alterRetention, &QAction::triggered, this, &MainWindow::alterRetentionPolicy);
    m_dropRetention = new QAction(tr("删除 Retention Policy…"), this);
    connect(m_dropRetention, &QAction::triggered, this, &MainWindow::dropRetentionPolicy);
    m_continuousQueries = new QAction(tr("Continuous Queries"), this);
    connect(m_continuousQueries, &QAction::triggered, this, &MainWindow::showContinuousQueries);
    m_createContinuousQuery = new QAction(tr("创建 Continuous Query…"), this);
    connect(m_createContinuousQuery, &QAction::triggered, this, &MainWindow::createContinuousQuery);
    m_dropContinuousQuery = new QAction(tr("删除 Continuous Query…"), this);
    connect(m_dropContinuousQuery, &QAction::triggered, this, &MainWindow::dropContinuousQuery);
    m_backfill = new QAction(tr("Backfill…"), this);
    connect(m_backfill, &QAction::triggered, this, &MainWindow::backfill);

    m_users = new QAction(tr("用户与权限"), this);
    connect(m_users, &QAction::triggered, this, &MainWindow::showUsers);
    m_runningQueries = new QAction(tr("运行中的查询"), this);
    connect(m_runningQueries, &QAction::triggered, this, &MainWindow::showRunningQueries);
    m_diagnostics = new QAction(tr("Diagnostics"), this);
    connect(m_diagnostics, &QAction::triggered, this, &MainWindow::showDiagnostics);
    m_stats = new QAction(tr("Stats"), this);
    connect(m_stats, &QAction::triggered, this, &MainWindow::showStats);
    m_killQuery = new QAction(tr("Kill Query…"), this);
    connect(m_killQuery, &QAction::triggered, this, &MainWindow::killQuery);
    m_createUser = new QAction(tr("创建用户…"), this);
    connect(m_createUser, &QAction::triggered, this, &MainWindow::createUser);
    m_dropUser = new QAction(tr("删除用户…"), this);
    connect(m_dropUser, &QAction::triggered, this, &MainWindow::dropUser);
    m_changePassword = new QAction(tr("修改用户密码…"), this);
    connect(m_changePassword, &QAction::triggered, this, &MainWindow::changeUserPassword);
    m_grantPrivilege = new QAction(tr("授予用户权限…"), this);
    connect(m_grantPrivilege, &QAction::triggered, this, &MainWindow::grantUserPrivilege);
    m_showPrivileges = new QAction(tr("查看用户权限…"), this);
    connect(m_showPrivileges, &QAction::triggered, this, &MainWindow::showUserPrivileges);
    m_grantAdmin = new QAction(tr("授予管理员权限…"), this);
    connect(m_grantAdmin, &QAction::triggered, this, &MainWindow::grantAdministrator);
    m_revokeAdmin = new QAction(tr("撤销管理员权限…"), this);
    connect(m_revokeAdmin, &QAction::triggered, this, &MainWindow::revokeAdministrator);
    m_revokePrivilege = new QAction(tr("撤销用户权限…"), this);
    connect(m_revokePrivilege, &QAction::triggered, this, &MainWindow::revokeUserPrivilege);
    m_serverCommand = new QAction(tr("执行 InfluxQL 命令…"), this);
    connect(m_serverCommand, &QAction::triggered, this, &MainWindow::executeServerCommand);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(m_manageConnections);
    fileMenu->addAction(m_disconnect);
    fileMenu->addSeparator();
    fileMenu->addAction(m_importSettings);
    fileMenu->addAction(m_exportSettings);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), qApp, &QApplication::quit);

    QMenu *queryMenu = menuBar()->addMenu(tr("查询"));
    queryMenu->addAction(m_newQuery);
    queryMenu->addAction(m_runQuery);
    queryMenu->addAction(m_refresh);

    QMenu *databaseMenu = menuBar()->addMenu(tr("数据库"));
    databaseMenu->addAction(m_createDatabase);
    databaseMenu->addAction(m_dropSelected);
    databaseMenu->addAction(m_dropSeries);
    databaseMenu->addSeparator();
    databaseMenu->addAction(m_showSeries);
    databaseMenu->addAction(m_showTagKeys);
    databaseMenu->addAction(m_showTagValues);
    databaseMenu->addAction(m_showFieldKeys);
    databaseMenu->addSeparator();
    databaseMenu->addAction(m_retentionPolicies);
    databaseMenu->addAction(m_createRetention);
    databaseMenu->addAction(m_alterRetention);
    databaseMenu->addAction(m_dropRetention);
    databaseMenu->addAction(m_continuousQueries);
    databaseMenu->addAction(m_createContinuousQuery);
    databaseMenu->addAction(m_dropContinuousQuery);
    databaseMenu->addAction(m_backfill);

    QMenu *serverMenu = menuBar()->addMenu(tr("服务器"));
    serverMenu->addAction(m_users);
    serverMenu->addAction(m_createUser);
    serverMenu->addAction(m_dropUser);
    serverMenu->addAction(m_changePassword);
    serverMenu->addAction(m_grantPrivilege);
    serverMenu->addAction(m_showPrivileges);
    serverMenu->addAction(m_grantAdmin);
    serverMenu->addAction(m_revokeAdmin);
    serverMenu->addAction(m_revokePrivilege);
    serverMenu->addAction(m_runningQueries);
    serverMenu->addAction(m_killQuery);
    serverMenu->addAction(m_diagnostics);
    serverMenu->addAction(m_stats);
    serverMenu->addSeparator();
    serverMenu->addAction(m_serverCommand);

    QMenu *helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于、隐私与开源许可"), this, &MainWindow::about);

    QToolBar *toolbar = addToolBar(tr("主工具栏"));
    toolbar->setMovable(false);
    toolbar->addAction(m_manageConnections);
    toolbar->addAction(m_newQuery);
    toolbar->addAction(m_runQuery);
    toolbar->addAction(m_refresh);
    toolbar->addAction(m_disconnect);
    toolbar->addAction(tr("隐私与开源许可"), this, &MainWindow::about);
}

QString MainWindow::settingsPath() const
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    return directory + QStringLiteral("/connections.json");
}

void MainWindow::loadSettings()
{
    QFile file(settingsPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        statusBar()->showMessage(tr("连接配置读取失败：%1").arg(parseError.errorString()));
        return;
    }
    QJsonArray connections;
    if (document.isArray()) {
        connections = document.array();
    } else {
        connections = document.object().value(QStringLiteral("connections")).toArray();
    }
    for (const QJsonValue &value : connections) {
        ConnectionProfile profile = ConnectionProfile::fromJson(value.toObject());
        if (profile.id <= 0) {
            profile.id = m_nextProfileId;
        }
        m_nextProfileId = std::max(m_nextProfileId, profile.id + 1);
        m_profiles.insert(profile.id, profile);
    }
}

void MainWindow::saveSettings() const
{
    QJsonArray connections;
    QList<int> ids = m_profiles.keys();
    std::sort(ids.begin(), ids.end());
    for (const int id : ids) {
        connections.append(m_profiles.value(id).toJson());
    }
    QFile file(settingsPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(connections).toJson(QJsonDocument::Indented));
    }
}

void MainWindow::rebuildTree()
{
    m_tree->clear();
    QList<int> ids = m_profiles.keys();
    std::sort(ids.begin(), ids.end());
    for (const int id : ids) {
        addProfileToTree(m_profiles.value(id));
    }
    m_tree->expandToDepth(0);
}

void MainWindow::addProfileToTree(const ConnectionProfile &profile)
{
    auto *item = new QTreeWidgetItem(m_tree, QStringList{profile.name});
    item->setData(0, NodeTypeRole, static_cast<int>(NodeType::Connection));
    item->setData(0, ProfileIdRole, profile.id);
    item->setToolTip(0, profile.baseUrl());
    item->addChild(new QTreeWidgetItem(QStringList{tr("展开以加载数据库")}));
}

QTreeWidgetItem *MainWindow::selectedItem() const
{
    return m_tree ? m_tree->currentItem() : nullptr;
}

int MainWindow::selectedProfileId() const
{
    const QTreeWidgetItem *item = selectedItem();
    if (!item) {
        return m_profiles.isEmpty() ? 0 : m_profiles.constBegin().key();
    }
    QTreeWidgetItem *cursor = const_cast<QTreeWidgetItem *>(item);
    while (cursor) {
        const QVariant id = cursor->data(0, ProfileIdRole);
        if (id.isValid()) {
            return id.toInt();
        }
        cursor = cursor->parent();
    }
    return m_profiles.isEmpty() ? 0 : m_profiles.constBegin().key();
}

QString MainWindow::selectedDatabase() const
{
    const QTreeWidgetItem *item = selectedItem();
    if (!item) {
        const int id = selectedProfileId();
        return id > 0 ? m_profiles.value(id).database : QString();
    }
    const QVariant database = item->data(0, DatabaseRole);
    if (database.isValid()) {
        return database.toString();
    }
    const int id = selectedProfileId();
    return id > 0 ? m_profiles.value(id).database : QString();
}

QString MainWindow::selectedMeasurement() const
{
    const QTreeWidgetItem *item = selectedItem();
    return item ? item->data(0, MeasurementRole).toString() : QString();
}

InfluxClient *MainWindow::clientFor(int profileId)
{
    if (profileId <= 0 || !m_profiles.contains(profileId)) {
        return nullptr;
    }
    if (!m_clients.contains(profileId)) {
        m_clients.insert(profileId, new InfluxClient(m_profiles.value(profileId), this));
    }
    return m_clients.value(profileId);
}

InfluxClient *MainWindow::clientForSelection()
{
    return clientFor(selectedProfileId());
}

void MainWindow::treeExpanded(QTreeWidgetItem *item)
{
    if (!item) return;
    const NodeType type = static_cast<NodeType>(item->data(0, NodeTypeRole).toInt());
    if (type == NodeType::Connection) {
        loadDatabases(item);
    } else if (type == NodeType::Database) {
        loadMeasurements(item);
    }
}

void MainWindow::loadDatabases(QTreeWidgetItem *connectionItem)
{
    const int profileId = connectionItem->data(0, ProfileIdRole).toInt();
    InfluxClient *client = clientFor(profileId);
    if (!client) return;
    connectionItem->takeChildren();
    auto *loading = new QTreeWidgetItem(QStringList{tr("正在加载数据库…")});
    connectionItem->addChild(loading);
    statusBar()->showMessage(tr("正在加载数据库…"));
    client->query(QString(), QStringLiteral("SHOW DATABASES"),
                  [this, connectionItem](bool success, const QJsonDocument &document, const QString &error) {
        connectionItem->takeChildren();
        if (!success) {
            connectionItem->addChild(new QTreeWidgetItem(QStringList{tr("加载失败：%1").arg(error)}));
            notifyResult(false, error);
            return;
        }
        const TableData data = tableDataFromInflux(document);
        const int profileId = connectionItem->data(0, ProfileIdRole).toInt();
        for (const QStringList &row : data.rows) {
            const QString name = textFromTableRow(data.columns, row, QStringLiteral("name"));
            if (name.isEmpty()) continue;
            auto *database = new QTreeWidgetItem(QStringList{name});
            database->setData(0, NodeTypeRole, static_cast<int>(NodeType::Database));
            database->setData(0, ProfileIdRole, profileId);
            database->setData(0, DatabaseRole, name);
            database->addChild(new QTreeWidgetItem(QStringList{tr("展开以加载 measurements")}));
            connectionItem->addChild(database);
        }
        notifyResult(true);
    });
}

void MainWindow::loadMeasurements(QTreeWidgetItem *databaseItem)
{
    const int profileId = databaseItem->data(0, ProfileIdRole).toInt();
    const QString database = databaseItem->data(0, DatabaseRole).toString();
    InfluxClient *client = clientFor(profileId);
    if (!client) return;
    databaseItem->takeChildren();
    auto *loading = new QTreeWidgetItem(QStringList{tr("正在加载 measurements…")});
    databaseItem->addChild(loading);
    client->query(database, QStringLiteral("SHOW MEASUREMENTS"),
                  [this, databaseItem, profileId, database](bool success, const QJsonDocument &document, const QString &error) {
        databaseItem->takeChildren();
        if (!success) {
            databaseItem->addChild(new QTreeWidgetItem(QStringList{tr("加载失败：%1").arg(error)}));
            notifyResult(false, error);
            return;
        }
        const TableData data = tableDataFromInflux(document);
        for (const QStringList &row : data.rows) {
            const QString name = textFromTableRow(data.columns, row, QStringLiteral("name"));
            if (name.isEmpty()) continue;
            auto *measurement = new QTreeWidgetItem(QStringList{name});
            measurement->setData(0, NodeTypeRole, static_cast<int>(NodeType::Measurement));
            measurement->setData(0, ProfileIdRole, profileId);
            measurement->setData(0, DatabaseRole, database);
            measurement->setData(0, MeasurementRole, name);
            databaseItem->addChild(measurement);
        }
        notifyResult(true);
    });
}

void MainWindow::openQuery(const QString &database, const QString &query)
{
    InfluxClient *client = clientForSelection();
    if (!client) {
        manageConnections();
        client = clientForSelection();
    }
    if (!client) return;

    auto *page = new QueryPage(client, database, m_tabs);
    if (!query.isEmpty()) {
        page->setQuery(query);
    }
    const int welcomeIndex = m_tabs->indexOf(m_tabs->findChild<QLabel *>(QStringLiteral("welcome")));
    if (welcomeIndex >= 0) {
        QWidget *welcome = m_tabs->widget(welcomeIndex);
        m_tabs->removeTab(welcomeIndex);
        welcome->deleteLater();
    }
    const QString tabTitle = database.isEmpty() ? tr("查询") : tr("查询 · %1").arg(database);
    m_tabs->addTab(page, tabTitle);
    m_tabs->setCurrentWidget(page);
}

void MainWindow::openTable(const QString &title, InfluxClient *client, const QString &database,
                           const QString &query)
{
    if (!client) return;
    auto *page = new TablePage(title, m_tabs);
    const int welcomeIndex = m_tabs->indexOf(m_tabs->findChild<QLabel *>(QStringLiteral("welcome")));
    if (welcomeIndex >= 0) {
        QWidget *welcome = m_tabs->widget(welcomeIndex);
        m_tabs->removeTab(welcomeIndex);
        welcome->deleteLater();
    }
    m_tabs->addTab(page, title);
    m_tabs->setCurrentWidget(page);
    page->setLoading(true);
    QPointer<TablePage> guardedPage(page);
    client->query(database, query, [guardedPage](bool success, const QJsonDocument &document, const QString &error) {
        if (!guardedPage) return;
        if (success) guardedPage->setDocument(document);
        else guardedPage->setError(error);
    });
}

void MainWindow::runQueryOnSelection(const QString &query, const QString &title)
{
    openTable(title, clientForSelection(), selectedDatabase(), query);
}

void MainWindow::newQuery()
{
    openQuery(selectedDatabase());
}

void MainWindow::runCurrentQuery()
{
    if (auto *page = qobject_cast<QueryPage *>(m_tabs->currentWidget())) {
        page->run();
    }
}

void MainWindow::refreshSelected()
{
    QTreeWidgetItem *item = selectedItem();
    if (!item) {
        rebuildTree();
        return;
    }
    const NodeType type = static_cast<NodeType>(item->data(0, NodeTypeRole).toInt());
    if (type == NodeType::Connection) {
        loadDatabases(item);
    } else if (type == NodeType::Database) {
        loadMeasurements(item);
    } else if (type == NodeType::Measurement) {
        runQueryOnSelection(QStringLiteral("SELECT * FROM %1 LIMIT 100").arg(quotedIdentifier(selectedMeasurement())),
                            tr("数据 · %1").arg(selectedMeasurement()));
    } else if (auto *page = qobject_cast<QueryPage *>(m_tabs->currentWidget())) {
        page->run();
    }
}

void MainWindow::disconnectSelected()
{
    const int profileId = selectedProfileId();
    if (profileId <= 0) return;
    if (m_clients.contains(profileId)) {
        delete m_clients.take(profileId);
    }
    if (QTreeWidgetItem *item = selectedItem()) {
        const NodeType type = static_cast<NodeType>(item->data(0, NodeTypeRole).toInt());
        if (type == NodeType::Connection) {
            item->takeChildren();
            item->addChild(new QTreeWidgetItem(QStringList{tr("已断开 · 展开以重新连接")}));
            item->setExpanded(false);
        }
    }
    statusBar()->showMessage(tr("已断开连接"), 4000);
}

void MainWindow::treeDoubleClicked(QTreeWidgetItem *item, int)
{
    if (!item) return;
    const NodeType type = static_cast<NodeType>(item->data(0, NodeTypeRole).toInt());
    if (type == NodeType::Connection) {
        loadDatabases(item);
    } else if (type == NodeType::Database) {
        openQuery(item->data(0, DatabaseRole).toString(), QStringLiteral("SHOW MEASUREMENTS"));
    } else if (type == NodeType::Measurement) {
        openQuery(item->data(0, DatabaseRole).toString(),
                  QStringLiteral("SELECT * FROM %1 LIMIT 100").arg(quotedIdentifier(selectedMeasurement())));
    }
}

void MainWindow::showUsers()
{
    runQueryOnSelection(QStringLiteral("SHOW USERS"), tr("用户与权限"));
}

void MainWindow::showRunningQueries()
{
    runQueryOnSelection(QStringLiteral("SHOW QUERIES"), tr("运行中的查询"));
}

void MainWindow::showDiagnostics()
{
    runQueryOnSelection(QStringLiteral("SHOW DIAGNOSTICS"), tr("Diagnostics"));
}

void MainWindow::showStats()
{
    runQueryOnSelection(QStringLiteral("SHOW STATS"), tr("Stats"));
}

void MainWindow::showSeries()
{
    const QString measurement = selectedMeasurement();
    const QString query = measurement.isEmpty()
                              ? QStringLiteral("SHOW SERIES")
                              : QStringLiteral("SHOW SERIES FROM %1").arg(quotedIdentifier(measurement));
    runQueryOnSelection(query, tr("Series"));
}

void MainWindow::showTagKeys()
{
    const QString measurement = selectedMeasurement();
    if (measurement.isEmpty()) {
        QMessageBox::information(this, tr("Tag Keys"), tr("请先选择 measurement。"));
        return;
    }
    runQueryOnSelection(QStringLiteral("SHOW TAG KEYS FROM %1").arg(quotedIdentifier(measurement)), tr("Tag Keys"));
}

void MainWindow::showTagValues()
{
    const QString measurement = selectedMeasurement();
    if (measurement.isEmpty()) {
        QMessageBox::information(this, tr("Tag Values"), tr("请先选择 measurement。"));
        return;
    }
    bool ok = false;
    const QString tag = QInputDialog::getText(this, tr("Tag Values"), tr("Tag 名称："),
                                               QLineEdit::Normal, QString(), &ok).trimmed();
    if (ok && !tag.isEmpty()) {
        runQueryOnSelection(QStringLiteral("SHOW TAG VALUES FROM %1 WITH KEY = %2")
                                .arg(quotedIdentifier(measurement), quotedIdentifier(tag)),
                            tr("Tag Values · %1").arg(tag));
    }
}

void MainWindow::showFieldKeys()
{
    const QString measurement = selectedMeasurement();
    if (measurement.isEmpty()) {
        QMessageBox::information(this, tr("Field Keys"), tr("请先选择 measurement。"));
        return;
    }
    runQueryOnSelection(QStringLiteral("SHOW FIELD KEYS FROM %1").arg(quotedIdentifier(measurement)), tr("Field Keys"));
}

void MainWindow::showRetentionPolicies()
{
    const QString database = selectedDatabase();
    if (database.isEmpty()) {
        QMessageBox::information(this, tr("Retention Policies"), tr("请先选择数据库。"));
        return;
    }
    runQueryOnSelection(QStringLiteral("SHOW RETENTION POLICIES ON %1").arg(quotedIdentifier(database)),
                        tr("Retention Policies · %1").arg(database));
}

void MainWindow::createRetentionPolicy()
{
    const QString database = selectedDatabase();
    InfluxClient *client = clientForSelection();
    if (database.isEmpty() || !client) {
        QMessageBox::information(this, tr("创建 Retention Policy"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("创建 Retention Policy"), tr("策略名称："),
                                                QLineEdit::Normal, QStringLiteral("autogen"), &ok).trimmed();
    if (!ok || name.isEmpty()) return;
    const QString duration = QInputDialog::getText(this, tr("创建 Retention Policy"), tr("保留时长（如 168h）："),
                                                    QLineEdit::Normal, QStringLiteral("0s"), &ok).trimmed();
    if (!ok || duration.isEmpty()) return;
    const int replication = QInputDialog::getInt(this, tr("创建 Retention Policy"), tr("Replication："), 1, 1, 100, 1, &ok);
    if (!ok) return;
    const QString defaultChoice = QInputDialog::getItem(this, tr("创建 Retention Policy"), tr("设为默认策略："),
                                                         {tr("否"), tr("是")}, 0, false, &ok);
    if (!ok) return;
    QString statement = QStringLiteral("CREATE RETENTION POLICY %1 ON %2 DURATION %3 REPLICATION %4")
                            .arg(quotedIdentifier(name), quotedIdentifier(database), duration).arg(replication);
    if (defaultChoice == tr("是")) statement += QStringLiteral(" DEFAULT");
    client->postQuery(database, statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
    });
}

void MainWindow::alterRetentionPolicy()
{
    const QString database = selectedDatabase();
    InfluxClient *client = clientForSelection();
    if (database.isEmpty() || !client) {
        QMessageBox::information(this, tr("修改 Retention Policy"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("修改 Retention Policy"), tr("策略名称："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) return;
    const QString duration = QInputDialog::getText(this, tr("修改 Retention Policy"), tr("新保留时长："),
                                                    QLineEdit::Normal, QStringLiteral("168h"), &ok).trimmed();
    if (!ok || duration.isEmpty()) return;
    const int replication = QInputDialog::getInt(this, tr("修改 Retention Policy"), tr("Replication："),
                                                  1, 1, 100, 1, &ok);
    if (!ok) return;
    const QString statement = QStringLiteral("ALTER RETENTION POLICY %1 ON %2 DURATION %3 REPLICATION %4")
                                  .arg(quotedIdentifier(name), quotedIdentifier(database), duration).arg(replication);
    client->postQuery(database, statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
    });
}

void MainWindow::dropRetentionPolicy()
{
    const QString database = selectedDatabase();
    InfluxClient *client = clientForSelection();
    if (database.isEmpty() || !client) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("删除 Retention Policy"), tr("策略名称："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) return;
    if (QMessageBox::question(this, tr("确认删除"), tr("确定删除策略 %1？").arg(name)) != QMessageBox::Yes) return;
    client->postQuery(database, QStringLiteral("DROP RETENTION POLICY %1 ON %2")
                                    .arg(quotedIdentifier(name), quotedIdentifier(database)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::showContinuousQueries()
{
    const QString database = selectedDatabase();
    if (database.isEmpty()) {
        QMessageBox::information(this, tr("Continuous Queries"), tr("请先选择数据库。"));
        return;
    }
    runQueryOnSelection(QStringLiteral("SHOW CONTINUOUS QUERIES"), tr("Continuous Queries · %1").arg(database));
}

void MainWindow::createContinuousQuery()
{
    const QString database = selectedDatabase();
    InfluxClient *client = clientForSelection();
    if (database.isEmpty() || !client) {
        QMessageBox::information(this, tr("创建 Continuous Query"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("创建 Continuous Query"), tr("CQ 名称："),
                                                QLineEdit::Normal, QStringLiteral("cq_5m"), &ok).trimmed();
    if (!ok || name.isEmpty()) return;
    const QString source = QInputDialog::getText(this, tr("创建 Continuous Query"), tr("源 measurement："),
                                                  QLineEdit::Normal, QStringLiteral("measurement"), &ok).trimmed();
    if (!ok || source.isEmpty()) return;
    const QString destination = QInputDialog::getText(this, tr("创建 Continuous Query"), tr("目标 measurement："),
                                                       QLineEdit::Normal, source + QStringLiteral("_5m"), &ok).trimmed();
    if (!ok || destination.isEmpty()) return;
    const QString interval = QInputDialog::getText(this, tr("创建 Continuous Query"), tr("聚合周期："),
                                                    QLineEdit::Normal, QStringLiteral("5m"), &ok).trimmed();
    if (!ok || interval.isEmpty()) return;

    const QString statement = QStringLiteral("CREATE CONTINUOUS QUERY %1 ON %2 BEGIN SELECT mean(*) INTO %3 FROM %4 GROUP BY time(%5),* END")
                                  .arg(quotedIdentifier(name), quotedIdentifier(database), quotedIdentifier(destination),
                                       quotedIdentifier(source), interval);
    client->postQuery(database, statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
    });
}

void MainWindow::dropContinuousQuery()
{
    const QString database = selectedDatabase();
    InfluxClient *client = clientForSelection();
    if (database.isEmpty() || !client) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("删除 Continuous Query"), tr("CQ 名称："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) return;
    client->postQuery(database, QStringLiteral("DROP CONTINUOUS QUERY %1 ON %2")
                                    .arg(quotedIdentifier(name), quotedIdentifier(database)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::backfill()
{
    const QString database = selectedDatabase();
    InfluxClient *client = clientForSelection();
    if (database.isEmpty() || !client) {
        QMessageBox::information(this, tr("Backfill"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString source = QInputDialog::getText(this, tr("Backfill"), tr("源 measurement："),
                                                  QLineEdit::Normal, selectedMeasurement(), &ok).trimmed();
    if (!ok || source.isEmpty()) return;
    const QString destination = QInputDialog::getText(this, tr("Backfill"), tr("目标 measurement："),
                                                       QLineEdit::Normal, source + QStringLiteral("_backfill"), &ok).trimmed();
    if (!ok || destination.isEmpty()) return;
    const QString start = QInputDialog::getText(this, tr("Backfill"), tr("开始时间（RFC3339）："),
                                                 QLineEdit::Normal, QStringLiteral("2024-01-01T00:00:00Z"), &ok).trimmed();
    if (!ok || start.isEmpty()) return;
    const QString end = QInputDialog::getText(this, tr("Backfill"), tr("结束时间（RFC3339）："),
                                               QLineEdit::Normal, QStringLiteral("2024-01-02T00:00:00Z"), &ok).trimmed();
    if (!ok || end.isEmpty()) return;
    const QString statement = QStringLiteral("SELECT * INTO %1 FROM %2 WHERE time >= '%3' AND time < '%4' GROUP BY *")
                                  .arg(quotedIdentifier(destination), quotedIdentifier(source), start, end);
    client->postQuery(database, statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
    });
}

void MainWindow::createDatabase()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString database = QInputDialog::getText(this, tr("创建数据库"), tr("数据库名称："),
                                                    QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || database.isEmpty()) return;
    client->postQuery(QString(), QStringLiteral("CREATE DATABASE %1").arg(quotedIdentifier(database)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                          if (success) refreshSelected();
                      });
}

void MainWindow::killQuery()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const int pid = QInputDialog::getInt(this, tr("Kill Query"), tr("查询 PID："), 0, 1, INT_MAX, 1, &ok);
    if (!ok) return;
    client->postQuery(QString(), QStringLiteral("KILL QUERY %1").arg(pid),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::createUser()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("创建用户"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    const QString password = QInputDialog::getText(this, tr("创建用户"), tr("密码："),
                                                    QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    const QString adminChoice = QInputDialog::getItem(this, tr("创建用户"), tr("管理员权限："),
                                                       {tr("普通用户"), tr("管理员")}, 0, false, &ok);
    if (!ok) return;
    QString statement = QStringLiteral("CREATE USER %1 WITH PASSWORD %2")
                            .arg(quotedIdentifier(user), quotedLiteral(password));
    if (adminChoice == tr("管理员")) statement += QStringLiteral(" WITH ALL PRIVILEGES");
    client->postQuery(QString(), statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
    });
}

void MainWindow::dropUser()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("删除用户"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    if (QMessageBox::question(this, tr("确认删除"), tr("确定删除用户 %1？").arg(user)) != QMessageBox::Yes) return;
    client->postQuery(QString(), QStringLiteral("DROP USER %1").arg(quotedIdentifier(user)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::changeUserPassword()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("修改密码"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    const QString password = QInputDialog::getText(this, tr("修改密码"), tr("新密码："),
                                                    QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    client->postQuery(QString(), QStringLiteral("SET PASSWORD FOR %1 = %2")
                                    .arg(quotedIdentifier(user), quotedLiteral(password)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::grantUserPrivilege()
{
    InfluxClient *client = clientForSelection();
    const QString database = selectedDatabase();
    if (!client || database.isEmpty()) {
        QMessageBox::information(this, tr("授予权限"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("授予权限"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    const QString privilege = QInputDialog::getItem(this, tr("授予权限"), tr("权限："),
                                                     {QStringLiteral("READ"), QStringLiteral("WRITE"), QStringLiteral("ALL")},
                                                     0, false, &ok);
    if (!ok) return;
    client->postQuery(QString(), QStringLiteral("GRANT %1 ON %2 TO %3")
                                    .arg(privilege, quotedIdentifier(database), quotedIdentifier(user)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::showUserPrivileges()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("查看用户权限"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (ok && !user.isEmpty()) {
        openTable(tr("用户权限 · %1").arg(user), client, QString(),
                  QStringLiteral("SHOW GRANTS FOR %1").arg(quotedIdentifier(user)));
    }
}

void MainWindow::grantAdministrator()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("授予管理员权限"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    client->postQuery(QString(), QStringLiteral("GRANT ALL PRIVILEGES TO %1").arg(quotedIdentifier(user)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::revokeAdministrator()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("撤销管理员权限"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    client->postQuery(QString(), QStringLiteral("REVOKE ALL PRIVILEGES FROM %1").arg(quotedIdentifier(user)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::revokeUserPrivilege()
{
    InfluxClient *client = clientForSelection();
    const QString database = selectedDatabase();
    if (!client || database.isEmpty()) {
        QMessageBox::information(this, tr("撤销用户权限"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString user = QInputDialog::getText(this, tr("撤销用户权限"), tr("用户名："),
                                                QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || user.isEmpty()) return;
    const QString privilege = QInputDialog::getItem(this, tr("撤销用户权限"), tr("权限："),
                                                     {QStringLiteral("READ"), QStringLiteral("WRITE"), QStringLiteral("ALL")},
                                                     0, false, &ok);
    if (!ok) return;
    client->postQuery(QString(), QStringLiteral("REVOKE %1 ON %2 FROM %3")
                                    .arg(privilege, quotedIdentifier(database), quotedIdentifier(user)),
                      [this](bool success, const QJsonDocument &, const QString &error) {
                          notifyResult(success, error);
                      });
}

void MainWindow::dropSelected()
{
    QTreeWidgetItem *item = selectedItem();
    InfluxClient *client = clientForSelection();
    if (!item || !client) return;
    const NodeType type = static_cast<NodeType>(item->data(0, NodeTypeRole).toInt());
    QString prompt;
    QString statement;
    if (type == NodeType::Database) {
        const QString database = selectedDatabase();
        prompt = tr("确定删除数据库 %1？此操作不可撤销。").arg(database);
        statement = QStringLiteral("DROP DATABASE %1").arg(quotedIdentifier(database));
    } else if (type == NodeType::Measurement) {
        const QString measurement = selectedMeasurement();
        prompt = tr("确定删除 measurement %1？").arg(measurement);
        statement = QStringLiteral("DROP MEASUREMENT %1").arg(quotedIdentifier(measurement));
    } else {
        QMessageBox::information(this, tr("删除资源"), tr("请在左侧选择数据库或 measurement。"));
        return;
    }
    if (QMessageBox::question(this, tr("确认删除"), prompt) != QMessageBox::Yes) return;
    client->postQuery(selectedDatabase(), statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
        if (success) refreshSelected();
    });
}

void MainWindow::dropSeries()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    const QString database = selectedDatabase();
    if (database.isEmpty()) {
        QMessageBox::information(this, tr("删除 Series"), tr("请先选择数据库。"));
        return;
    }
    bool ok = false;
    const QString measurement = QInputDialog::getText(this, tr("删除 Series"),
                                                       tr("measurement（留空则删除数据库中所有 series）："),
                                                       QLineEdit::Normal, selectedMeasurement(), &ok).trimmed();
    if (!ok) return;
    const QString scope = measurement.isEmpty() ? database : measurement;
    if (QMessageBox::question(this, tr("确认删除"), tr("确定删除 %1 下的 series？").arg(scope)) != QMessageBox::Yes) return;
    const QString statement = measurement.isEmpty()
                                  ? QStringLiteral("DROP SERIES")
                                  : QStringLiteral("DROP SERIES FROM %1").arg(quotedIdentifier(measurement));
    client->postQuery(database, statement, [this](bool success, const QJsonDocument &, const QString &error) {
        notifyResult(success, error);
    });
}

void MainWindow::executeServerCommand()
{
    InfluxClient *client = clientForSelection();
    if (!client) return;
    bool ok = false;
    const QString statement = QInputDialog::getMultiLineText(this, tr("执行 InfluxQL"), tr("InfluxQL："),
                                                              QStringLiteral("SHOW DATABASES"), &ok).trimmed();
    if (ok && !statement.isEmpty()) {
        openTable(tr("命令结果"), client, selectedDatabase(), statement);
    }
}

void MainWindow::notifyResult(bool success, const QString &error)
{
    statusBar()->showMessage(success ? tr("操作完成") : tr("操作失败：%1").arg(error), 8000);
}

void MainWindow::manageConnections()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("管理 InfluxDB 连接"));
    dialog.resize(700, 420);
    auto *layout = new QVBoxLayout(&dialog);
    auto *table = new QTableWidget(&dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({tr("名称"), tr("地址"), tr("默认数据库")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table, 1);

    auto refreshRows = [this, table]() {
        table->setRowCount(0);
        QList<int> ids = m_profiles.keys();
        std::sort(ids.begin(), ids.end());
        for (const int id : ids) {
            const ConnectionProfile profile = m_profiles.value(id);
            const int row = table->rowCount();
            table->insertRow(row);
            auto *name = new QTableWidgetItem(profile.name);
            name->setData(Qt::UserRole, id);
            table->setItem(row, 0, name);
            table->setItem(row, 1, new QTableWidgetItem(profile.baseUrl()));
            table->setItem(row, 2, new QTableWidgetItem(profile.database));
        }
        if (table->rowCount() > 0) table->selectRow(0);
    };
    refreshRows();

    auto *buttons = new QHBoxLayout;
    auto *add = new QPushButton(tr("新增"), &dialog);
    auto *edit = new QPushButton(tr("编辑"), &dialog);
    auto *remove = new QPushButton(tr("删除"), &dialog);
    auto *test = new QPushButton(tr("测试连接"), &dialog);
    auto *connectButton = new QPushButton(tr("连接"), &dialog);
    auto *close = new QPushButton(tr("关闭"), &dialog);
    buttons->addWidget(add);
    buttons->addWidget(edit);
    buttons->addWidget(remove);
    buttons->addWidget(test);
    buttons->addStretch(1);
    buttons->addWidget(connectButton);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    auto selectedId = [table]() -> int {
        const int row = table->currentRow();
        return row >= 0 && table->item(row, 0) ? table->item(row, 0)->data(Qt::UserRole).toInt() : 0;
    };
    connect(add, &QPushButton::clicked, &dialog, [this, &dialog, refreshRows]() mutable {
        ConnectionDialog editor(&dialog);
        if (editor.exec() == QDialog::Accepted) {
            ConnectionProfile profile = editor.profile();
            profile.id = m_nextProfileId++;
            if (profile.name.isEmpty()) profile.name = profile.host;
            m_profiles.insert(profile.id, profile);
            saveSettings();
            rebuildTree();
            refreshRows();
        }
    });
    connect(edit, &QPushButton::clicked, &dialog, [this, &dialog, refreshRows, selectedId]() mutable {
        const int id = selectedId();
        if (!m_profiles.contains(id)) return;
        ConnectionDialog editor(&dialog);
        editor.setProfile(m_profiles.value(id));
        if (editor.exec() == QDialog::Accepted) {
            ConnectionProfile profile = editor.profile();
            profile.id = id;
            if (profile.name.isEmpty()) profile.name = profile.host;
            m_profiles.insert(id, profile);
            if (m_clients.contains(id)) {
                delete m_clients.take(id);
            }
            saveSettings();
            rebuildTree();
            refreshRows();
        }
    });
    connect(remove, &QPushButton::clicked, &dialog, [this, &dialog, refreshRows, selectedId]() mutable {
        const int id = selectedId();
        if (!m_profiles.contains(id)) return;
        if (QMessageBox::question(&dialog, tr("删除连接"), tr("确定删除这个连接配置？")) != QMessageBox::Yes) return;
        m_profiles.remove(id);
        if (m_clients.contains(id)) delete m_clients.take(id);
        saveSettings();
        rebuildTree();
        refreshRows();
    });
    connect(test, &QPushButton::clicked, &dialog, [this, &dialog, selectedId]() {
        const int id = selectedId();
        InfluxClient *client = clientFor(id);
        if (!client) return;
        client->ping([&dialog](bool success, const QJsonDocument &, const QString &error) {
            QMessageBox::information(&dialog, tr("连接测试"), success ? tr("连接成功。") : tr("连接失败：%1").arg(error));
        });
    });
    connect(connectButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(close, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(table, &QTableWidget::cellDoubleClicked, &dialog, &QDialog::accept);
    dialog.exec();
}

void MainWindow::importSettings()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("导入连接配置"), QString(), tr("JSON 文件 (*.json)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("导入失败"), file.errorString());
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("导入失败"), parseError.errorString());
        return;
    }
    const QJsonArray connections = document.isArray()
                                       ? document.array()
                                       : document.object().value(QStringLiteral("connections")).toArray();
    for (const QJsonValue &value : connections) {
        ConnectionProfile profile = ConnectionProfile::fromJson(value.toObject());
        profile.id = m_nextProfileId++;
        m_profiles.insert(profile.id, profile);
    }
    saveSettings();
    rebuildTree();
    statusBar()->showMessage(tr("已导入 %1 个连接配置").arg(connections.size()), 5000);
}

void MainWindow::exportSettings()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("导出连接配置"),
                                                       QStringLiteral("influxdb-connections.json"),
                                                       tr("JSON 文件 (*.json)"));
    if (path.isEmpty()) return;
    QJsonArray connections;
    QList<int> ids = m_profiles.keys();
    std::sort(ids.begin(), ids.end());
    for (const int id : ids) connections.append(m_profiles.value(id).toJson());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(connections).toJson(QJsonDocument::Indented));
        statusBar()->showMessage(tr("已导出连接配置"), 5000);
    } else {
        QMessageBox::warning(this, tr("导出失败"), file.errorString());
    }
}

void MainWindow::about()
{
    QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("SeriesLedger · 隐私与许可"));
    dialog.resize(1100, 780); auto *layout = new QVBoxLayout(&dialog);
    auto *text = new QPlainTextEdit(&dialog); text->setReadOnly(true);
    text->setPlainText(QString::fromUtf8(SERIESLEDGER_RELEASE_NOTICES)); layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject); layout->addWidget(buttons);
    dialog.exec();
}
