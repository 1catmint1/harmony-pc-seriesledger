#pragma once
#include "jsonhelpers.h"
#include "influxclient.h"
#ifndef SERIESLEDGER_CORE_ONLY
#include "querypage.h"
#include "tablepage.h"
#endif
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QLabel>
#include <QNetworkReply>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTableWidget>
#include <QTimer>
#include <QDebug>
#include <cstdio>
#include <stdexcept>

inline void validationRequire(bool condition, const char *name)
{ if (!condition) throw std::runtime_error(name); }

inline QJsonDocument validationSeries(const QJsonArray &columns, const QJsonArray &rows)
{
    QJsonObject series;series.insert(QStringLiteral("name"),QStringLiteral("fixture"));series.insert(QStringLiteral("columns"),columns);series.insert(QStringLiteral("values"),rows);
    QJsonArray allSeries;allSeries.append(series);QJsonObject statement;statement.insert(QStringLiteral("statement_id"),0);statement.insert(QStringLiteral("series"),allSeries);
    QJsonArray statements;statements.append(statement);QJsonObject root;root.insert(QStringLiteral("results"),statements);return QJsonDocument(root);
}
inline QJsonArray validationRows(const QJsonArray &row)
{ QJsonArray rows;rows.append(QJsonValue(row));return rows; }

inline int runCoreValidation(const QJsonDocument &mixed, const QJsonDocument &later, const char *prefix)
{
    int passed=0;
    auto check=[&](bool ok,const char *name){validationRequire(ok,name);++passed;};
    const TableData data=tableDataFromInflux(mixed);
    check(data.error.isEmpty() && data.rows.size()==3,"real rows");
    check(data.columns.size()==8,"real schema union");
    check(data.rows.at(0).size()==8 && data.rows.at(2).size()==8,"aligned rows");
    auto cell=[&](int row,const QString &name){const int column=data.columns.indexOf(name);validationRequire(column>=0,"missing column");return data.rows.at(row).at(column);};
    check(cell(0,"field.value")=="5.5" && cell(0,"field.bytes").isEmpty(),"cpu alignment");
    check(cell(2,"field.bytes")=="4096" && cell(2,"field.value").isEmpty(),"mem alignment");
    check(cell(0,"tag.host")=="pcA" && cell(1,"tag.host")=="pcB","tag provenance");
    check(cell(2,"statement_id")=="1" && cell(2,"measurement")=="mem","statement provenance");
    check(cell(0,"field.note")==QString::fromUtf8("甲,quoted"),"Unicode value");
    check(influxResponseError(later).startsWith("statement 1:"),"later real error");
    const TableData failed=tableDataFromInflux(later);
    check(!failed.error.isEmpty() && failed.rows.isEmpty() && failed.columns.isEmpty(),"no partial success table");
    check(influxResponseError(QJsonDocument(QJsonObject{{"error","root failure"}}))=="root failure","root error");
    check(!influxResponseError(QJsonDocument(QJsonObject())).isEmpty(),"missing envelope");
    check(!influxResponseError(QJsonDocument(QJsonArray())).isEmpty(),"array envelope");
    check(!influxResponseError(QJsonDocument(QJsonObject{{"results",QJsonArray{42}}})).isEmpty(),"invalid result object");
    check(!tableDataFromInflux(validationSeries(QJsonArray{QStringLiteral("x"),QStringLiteral("x")},validationRows(QJsonArray{1,2}))).error.isEmpty(),"duplicate fields");
    check(!tableDataFromInflux(validationSeries(QJsonArray{QStringLiteral("x")},validationRows(QJsonArray{1,2}))).error.isEmpty(),"malformed row");
    QJsonArray wide;for(int i=0;i<257;++i)wide.append(QString::number(i));
    check(!tableDataFromInflux(validationSeries(wide,QJsonArray())).error.isEmpty(),"column budget");
    QJsonArray longRows;for(int i=0;i<10001;++i)longRows.append(QJsonValue(QJsonArray{i}));
    check(!tableDataFromInflux(validationSeries(QJsonArray{QStringLiteral("x")},longRows)).error.isEmpty(),"row budget");
    check(jsonValueToText(QJsonValue(QJsonValue::Null)).isEmpty(),"null rendering");
    check(jsonValueToText(QJsonValue(true))=="true","bool rendering");
    const double precise=1.2345678901234567;
    check(jsonValueToText(QJsonValue(precise)).toDouble()==precise,"double round trip");
    check(jsonValueToText(QJsonValue(QJsonArray{1,true})).contains("true"),"structured value");
    const TableData collision=tableDataFromInflux(validationSeries(QJsonArray{QStringLiteral("measurement"),QStringLiteral("statement_id")},validationRows(QJsonArray{QStringLiteral("value"),QStringLiteral("payload")})));
    if (!collision.error.isEmpty()) qWarning("%s CORE_FIXTURE error=%s",prefix,collision.error.toUtf8().constData());
    check(collision.columns.contains("field.measurement") && collision.columns.contains("measurement") && collision.columns.size()==4,"metadata field collision");
    qWarning("%s CORE|passed=%d|total=23",prefix,passed);
    validationRequire(passed==23,"core count");return passed;
}

struct ValidationReply { bool ok=false; QJsonDocument document; QString error; };
inline ValidationReply validationQuery(InfluxClient &client,const QString &db,const QString &q,bool post=false,bool ping=false)
{
    QEventLoop loop;QTimer deadline;deadline.setSingleShot(true);
    auto done=std::make_shared<bool>(false);auto result=std::make_shared<ValidationReply>();QPointer<QEventLoop> activeLoop(&loop);
    QObject::connect(&deadline,&QTimer::timeout,&loop,&QEventLoop::quit);
    auto callback=[done,result,activeLoop](bool ok,const QJsonDocument &document,const QString &error){*done=true;*result={ok,document,error};if(activeLoop)activeLoop->quit();};
    deadline.start(5000);
    if(ping)client.ping(callback);else if(post)client.postQuery(db,q,callback);else client.query(db,q,callback);
    loop.exec();validationRequire(*done,"real network completion timeout");return *result;
}

inline int runLiveValidation(const ConnectionProfile &profile,const char *prefix)
{
    int passed=0;auto check=[&](bool ok,const char *name){validationRequire(ok,name);++passed;};
    InfluxClient client(profile);
    auto response=validationQuery(client,{}, {},false,true);check(response.ok,"real ping");
    response=validationQuery(client,profile.database,"SELECT value,note FROM cpu GROUP BY host; SELECT bytes,ok FROM mem GROUP BY host");
    if (!response.ok) std::printf("%s DIAGNOSTIC %s\n",prefix,response.error.toUtf8().constData());
    auto table=tableDataFromInflux(response.document);check(response.ok && table.error.isEmpty() && table.rows.size()==3,"real Qt multi query");
    check(table.rows.at(0).at(table.columns.indexOf("field.time"))=="1700000000000","real epoch ms");
    response=validationQuery(client,{},"SHOW DATABASES; SELECT * FROM \"track1_missing_db\".\"autogen\".\"cpu\"");check(!response.ok && response.error.startsWith("statement 1:"),"real later error callback");
    ConnectionProfile wrong=profile;wrong.password="deliberately_wrong_fixture_password";InfluxClient bad(wrong);
    response=validationQuery(bad,profile.database,"SHOW MEASUREMENTS");check(!response.ok,"real auth failure");
    response=validationQuery(client,profile.database,QString::fromUtf8("SELECT text FROM encoding WHERE text='literal%2B+amp&汉'"),true);
    const TableData encoded=tableDataFromInflux(response.document);
    check(response.ok && encoded.rows.size()==1 && encoded.rows.at(0).at(encoded.columns.indexOf("field.text"))==QString::fromUtf8("literal%2B+amp&汉"),"real POST reserved encoding");
#ifndef SERIESLEDGER_CORE_ONLY
    TablePage view("control");view.setDocument(QJsonDocument(QJsonObject{{"results",QJsonArray{QJsonObject()}}}));view.setLoading(true);
    check(view.document().isNull() && !view.findChild<QTableWidget*>()->isEnabled(),"loading invalidates export");
    view.setError("controlled failure");check(view.document().isNull() && view.findChild<QTableWidget*>()->columnCount()==0 && view.findChild<QTableWidget*>()->isEnabled(),"error clears and enables");
    response=validationQuery(client,profile.database,"SHOW MEASUREMENTS");view.setDocument(response.document);
    check(response.ok && !view.document().isNull() && view.findChild<QTableWidget*>()->isEnabled(),"success enables table");
    auto waitPage=[&](const std::function<void()> &start,const std::function<void()> &change){QEventLoop loop;QTimer timer;timer.setSingleShot(true);bool completed=false;auto *network=client.findChild<QNetworkAccessManager*>();validationRequire(network!=nullptr,"owned network manager");auto connection=QObject::connect(network,&QNetworkAccessManager::finished,&loop,[&](QNetworkReply*){completed=true;loop.quit();});QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);timer.start(5000);start();change();loop.exec();QObject::disconnect(connection);validationRequire(completed,"page request completion");};
    QueryPage page(&client,profile.database);page.setQuery("SHOW MEASUREMENTS");
    waitPage([&]{page.run();},[&]{page.setQuery("SHOW DATABASES");});
    check(page.findChild<TablePage*>()->document().isNull() && page.findChild<QTableWidget*>()->isEnabled(),"stale edited input discarded");
    page.setQuery("SHOW MEASUREMENTS");waitPage([&]{page.run();},[&]{page.setDatabase("track1_missing_db");});
    check(page.findChild<TablePage*>()->document().isNull(),"stale database discarded");
    auto *closing=new QueryPage(&client,profile.database);QPointer<QueryPage> guard(closing);
    waitPage([&]{closing->run();},[&]{delete closing;});check(!guard,"closed page callback guarded");
    auto *ephemeral=new InfluxClient(profile);QueryPage disconnected(ephemeral,profile.database);delete ephemeral;disconnected.run();
    check(disconnected.findChild<QLabel*>("pageStatus")->text().contains(QString::fromUtf8("没有可用")),"destroyed client guarded");
    qWarning("%s LIVE|passed=%d|total=13",prefix,passed);validationRequire(passed==13,"live count");return passed;
#else
    qWarning("%s LIVE|passed=%d|total=6",prefix,passed);validationRequire(passed==6,"live count");return passed;
#endif
}
