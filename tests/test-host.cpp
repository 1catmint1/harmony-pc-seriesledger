#include "validationchecks.h"
#include <QCoreApplication>
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);
    try {
        validationRequire(argc>=4,"fixture paths required");
        auto read=[](const char *name){QFile file(QString::fromUtf8(name));validationRequire(file.open(QIODevice::ReadOnly),"fixture open");return QJsonDocument::fromJson(file.readAll());};
        const int core=runCoreValidation(read(argv[1]),read(argv[2]),"SERIESLEDGER_HOST");
        const QJsonObject privateConfig=read(argv[3]).object();ConnectionProfile profile;profile.host="127.0.0.1";profile.port=18086;profile.database=privateConfig.value("database").toString();profile.username=privateConfig.value("username").toString();profile.password=privateConfig.value("password").toString();
        const int live=runLiveValidation(profile,"SERIESLEDGER_HOST");std::printf("SERIESLEDGER_HOST SUMMARY|core=%d|live=%d\n",core,live);return 0;
    }catch(const std::exception &error){std::printf("SERIESLEDGER_HOST ERROR %s\n",error.what());return 1;}
}
