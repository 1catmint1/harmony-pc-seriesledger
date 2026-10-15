#include "appmodel.h"
#include <QJsonDocument>
#include <QCoreApplication>
#include <cstdio>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ConnectionProfile p; p.id=7; p.name="own fixture"; p.host="127.0.0.1";
    p.port=18086; p.database="fixture"; p.username="local-test";
    p.password="session-only-fixture"; p.useSsl=true;
    const auto stored=p.toJson();
    if(stored.contains("password") || QJsonDocument(stored).toJson().contains(p.password.toUtf8())) return 1;
    const auto restored=ConnectionProfile::fromJson(stored);
    if(!restored.password.isEmpty() || restored.name!=p.name || restored.host!=p.host || restored.port!=p.port || restored.database!=p.database || restored.username!=p.username || !restored.useSsl) return 2;
    auto legacy=stored; legacy.insert("password",p.password);
    const auto session=ConnectionProfile::fromJson(legacy);
    if(session.password!=p.password || session.toJson().contains("password") || p.password!="session-only-fixture") return 3;
    std::puts("SERIESLEDGER_PROFILE|passwordExcluded=1|metadataRoundTrip=1|legacySessionOnly=1|total=3");
    return 0;
}
