#pragma once

#include "appmodel.h"

#include <QHash>
#include <QMainWindow>

class QAction;
class InfluxClient;
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QueryPage;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void manageConnections();
    void importSettings();
    void exportSettings();
    void newQuery();
    void runCurrentQuery();
    void refreshSelected();
    void disconnectSelected();
    void showUsers();
    void showRunningQueries();
    void showDiagnostics();
    void showStats();
    void showSeries();
    void showTagKeys();
    void showTagValues();
    void showFieldKeys();
    void showRetentionPolicies();
    void createRetentionPolicy();
    void alterRetentionPolicy();
    void dropRetentionPolicy();
    void showContinuousQueries();
    void createContinuousQuery();
    void dropContinuousQuery();
    void backfill();
    void createDatabase();
    void dropSelected();
    void dropSeries();
    void executeServerCommand();
    void killQuery();
    void createUser();
    void dropUser();
    void changeUserPassword();
    void grantUserPrivilege();
    void showUserPrivileges();
    void grantAdministrator();
    void revokeAdministrator();
    void revokeUserPrivilege();
    void about();

private slots:
    void treeExpanded(QTreeWidgetItem *item);
    void treeDoubleClicked(QTreeWidgetItem *item, int column);

private:
    static constexpr int NodeTypeRole = Qt::UserRole;
    static constexpr int ProfileIdRole = Qt::UserRole + 1;
    static constexpr int DatabaseRole = Qt::UserRole + 2;
    static constexpr int MeasurementRole = Qt::UserRole + 3;

    void createActions();
    void createMenus();
    void loadSettings();
    void saveSettings() const;
    void rebuildTree();
    void addProfileToTree(const ConnectionProfile &profile);
    void loadDatabases(QTreeWidgetItem *connectionItem);
    void loadMeasurements(QTreeWidgetItem *databaseItem);
    void openQuery(const QString &database, const QString &query = QString());
    void openTable(const QString &title, InfluxClient *client, const QString &database,
                   const QString &query);
    void runQueryOnSelection(const QString &query, const QString &title);
    int selectedProfileId() const;
    QString selectedDatabase() const;
    QString selectedMeasurement() const;
    InfluxClient *clientFor(int profileId);
    InfluxClient *clientForSelection();
    QTreeWidgetItem *selectedItem() const;
    void notifyResult(bool success, const QString &error = QString());
    QString settingsPath() const;

    QHash<int, ConnectionProfile> m_profiles;
    QHash<int, InfluxClient *> m_clients;
    int m_nextProfileId = 1;
    QTreeWidget *m_tree = nullptr;
    QTabWidget *m_tabs = nullptr;
    QAction *m_manageConnections = nullptr;
    QAction *m_importSettings = nullptr;
    QAction *m_exportSettings = nullptr;
    QAction *m_newQuery = nullptr;
    QAction *m_runQuery = nullptr;
    QAction *m_refresh = nullptr;
    QAction *m_disconnect = nullptr;
    QAction *m_createDatabase = nullptr;
    QAction *m_dropSelected = nullptr;
    QAction *m_dropSeries = nullptr;
    QAction *m_showSeries = nullptr;
    QAction *m_showTagKeys = nullptr;
    QAction *m_showTagValues = nullptr;
    QAction *m_showFieldKeys = nullptr;
    QAction *m_retentionPolicies = nullptr;
    QAction *m_createRetention = nullptr;
    QAction *m_alterRetention = nullptr;
    QAction *m_dropRetention = nullptr;
    QAction *m_continuousQueries = nullptr;
    QAction *m_createContinuousQuery = nullptr;
    QAction *m_dropContinuousQuery = nullptr;
    QAction *m_backfill = nullptr;
    QAction *m_users = nullptr;
    QAction *m_runningQueries = nullptr;
    QAction *m_diagnostics = nullptr;
    QAction *m_stats = nullptr;
    QAction *m_killQuery = nullptr;
    QAction *m_createUser = nullptr;
    QAction *m_dropUser = nullptr;
    QAction *m_changePassword = nullptr;
    QAction *m_grantPrivilege = nullptr;
    QAction *m_showPrivileges = nullptr;
    QAction *m_grantAdmin = nullptr;
    QAction *m_revokeAdmin = nullptr;
    QAction *m_revokePrivilege = nullptr;
    QAction *m_serverCommand = nullptr;
};
