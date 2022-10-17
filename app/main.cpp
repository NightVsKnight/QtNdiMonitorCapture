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
        auto baseName = "QtNdiMonitorCapture_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    auto arguments = app.arguments();

    auto appModeDefault = App::AppMode::Monitor;
    auto appModes =  App::getAppModes().join('|');
    QCommandLineParser parser;
    QCommandLineOption optionMode(QStringList() << "m" << "mode", appModes, "mode", App::toString(appModeDefault));
    parser.addOption(optionMode);
    parser.parse(arguments);
    bool appModeOk;
    auto appMode = App::toAppMode(parser.value(optionMode), &appModeOk);

    //
    // hard codes to help debugging
    //
#if 0
#if 0
    appMode = App::AppMode::Capture;
#else
    appMode = App::AppMode::Monitor;
#endif
#endif

    int returnCode;

    auto pNdi = NdiWrapper::ndiGet();
    if (pNdi)
    {
        MainWindow w;

        switch(appMode)
        {
        case App::AppMode::Monitor:
            w.show();
            break;
        case App::AppMode::Capture:
            w.captureStart();
            break;
        }

        returnCode = app.exec();

        NdiWrapper::ndiDestroy();
    }
    else
    {
        QMessageBox msgBox(nullptr);
        msgBox.setWindowTitle(QObject::tr("NDI Runtime Not Found - %1 %2")
                              .arg(QApplication::applicationName())
                              .arg(QApplication::applicationVersion()));
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
