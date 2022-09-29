#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>

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
    auto uiLanguages = QLocale::system().uiLanguages();
    for (auto locale : uiLanguages) {
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
    if (appMode.isEmpty())
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

    int returnCode;

    auto pNdi = NdiWrapper::ndiGet();
    if (pNdi)
    {
        MainWindow w;

        if (appMode.compare("monitor", Qt::CaseInsensitive) == 0)
        {
            w.show();
        }
        else if (appMode.compare("capture", Qt::CaseInsensitive) == 0)
        {
            w.captureStart();
        }

        returnCode = app.exec();

        NdiWrapper::ndiDestroy();
    }
    else
    {
        QMessageBox msgBox(nullptr);
        msgBox.setWindowTitle(QObject::tr("NDI Runtime Not Found"));
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(QString(QObject::tr("The NDI Runtime cannot be found.<br>\n"
                               "<br>\n"
                               "Please download and install the NDI Runtime from:<br>\n"
                               "<a href=\"%1\">%1</a>.<br>\n"
                               "<br>\n"
                               "This application is useless without the NDI Runtime and will now close."))
                       .arg(NDILIB_REDIST_URL));
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        returnCode = 1;
    }

    qDebug() << "returnCode" << returnCode;
    return returnCode;
}
