#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QScopedPointer>

#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
#include <QtOhExtras/QOhWidgetHelper>
#endif

namespace {

#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
QScopedPointer<MainWindow> harmonyWindow;
QScopedPointer<QOhWidgetHelper> harmonyWindowHelper;
#endif

void configureApplication(QApplication &app)
{
    QCoreApplication::setApplicationName(QStringLiteral("SeriesLedger"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("SeriesLedger"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.3"));
    QCoreApplication::setOrganizationName(QStringLiteral("SeriesLedger"));

    QFont font;
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    font.setFamily(QStringLiteral("HarmonyOS Sans SC"));
    font.setPixelSize(18);
#else
    font.setFamily(QStringLiteral("Microsoft YaHei"));
    font.setPixelSize(13);
#endif
    app.setFont(font);
    app.setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f5f7fa; }"
        "QTreeWidget, QTableWidget, QPlainTextEdit { background: #ffffff; }"
        "QTreeWidget::item:selected, QTableWidget::item:selected { background: #d9e9ff; color: #15345b; }"
        "QLabel#welcome { color: #506070; font-size: 22px; padding: 40px; }"
        "QLabel#pageStatus { color: #41546a; font-weight: 600; }"
        "QToolBar { spacing: 8px; padding: 6px; }"
    ));
}

void setupWindow(MainWindow *window)
{
    if (!window) return;
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    window->resize(1680, 1050);
#else
    window->resize(1440, 900);
#endif
    window->show();
    window->raise();
    window->activateWindow();

#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    harmonyWindowHelper.reset(new QOhWidgetHelper(window, window));
    harmonyWindowHelper->setSupportedWindowModes(QOhWidgetHelper::FULL_SCREEN
                                                 | QOhWidgetHelper::SPLIT
                                                 | QOhWidgetHelper::FLOATING);
    harmonyWindowHelper->setWindowRectAutoSave(true);
#endif
}

}

#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
extern "C" int main(int argc, char *argv[])
{
    static int fallbackArgc = 1;
    static char appName[] = "SeriesLedger";
    static char *fallbackArgv[] = {appName, nullptr};

    QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    bool ownsApp = false;
    if (!app) {
        if (argc <= 0 || !argv) {
            argc = fallbackArgc;
            argv = fallbackArgv;
        }
        app = new QApplication(argc, argv);
        ownsApp = true;
    }
    configureApplication(*app);
    harmonyWindow.reset(new MainWindow);
    setupWindow(harmonyWindow.data());

    int exitCode = 0;
    if (ownsApp) {
        exitCode = app->exec();
        harmonyWindowHelper.reset();
        harmonyWindow.reset();
        delete app;
    }
    return exitCode;
}

extern "C" int qtmain(int argc, char *argv[])
{
    return main(argc, argv);
}
#else
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    configureApplication(app);
    MainWindow window;
    setupWindow(&window);
    return app.exec();
}
#endif

