#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QTranslator>
#include <QDebug>

#include "MainWindow.h"
#include "ndiwrapper.h"

using namespace winrt;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;

int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{time hh:mm:ss.zzz} p%{pid} t%{threadid} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{function}:%{line} | %{message}");

    QApplication app(argc, argv);

    winrt::uninit_apartment();
    winrt::init_apartment(apartment_type::single_threaded);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QtNdiMonitorCapture_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    // TODO: Just parse these directly in MainWindow?
    auto appModeDefault = "monitor";

    QCommandLineParser parser;
    QCommandLineOption optionMode(QStringList() << "m" << "mode", "\"monitor\"|\"capture\"", appModeDefault);
    parser.addOption(optionMode);
    parser.parse(app.arguments());
    auto appMode = parser.value(optionMode);
    if (appMode.isNull() || appMode.isEmpty())
    {
        appMode = appModeDefault;
    }

#if 0
#if 0
    appMode = "capture"; // <- debugging hard code
#else
    appMode = "monitor"; // <- debugging hard code
#endif
#endif

    auto *ndi = &NdiWrapper::get();

    ndi->ndiInitialize();

    MainWindow w;

    if (appMode.compare("monitor", Qt::CaseInsensitive) == 0)
    {
        w.show();
    }
    else if (appMode.compare("capture", Qt::CaseInsensitive) == 0)
    {
        w.captureStart();
    }

    auto returnCode = app.exec();

    ndi->ndiDestroy();

    qDebug() << "returnCode" << returnCode;
    return returnCode;
}
