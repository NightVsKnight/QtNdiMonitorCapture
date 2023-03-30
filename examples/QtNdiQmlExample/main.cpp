#include <QApplication>
#include <QQuickWidget>
#include <QQmlApplicationEngine>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    int returnCode = 0;

    QApplication app(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QtNdiQmlExample_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QQuickWidget mainWindow;
    mainWindow.engine()->addImportPath(QStringLiteral("../../plugins"));
    const QUrl url(u"qrc:/QtNdiQmlExample/main.qml"_qs);
    QQmlApplicationEngine engine = mainWindow.engine();
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    mainWindow.setResizeMode(QQuickWidget::SizeRootObjectToView);
    mainWindow.setSource(url);
    QObject::connect(mainWindow.engine(), &QQmlEngine::quit, &mainWindow, &QQuickWidget::close);
    mainWindow.show();
    returnCode = app.exec();

    qDebug() << "returnCode" << returnCode;
    return returnCode;
}
