#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QMenu>
#include <QVideoSink>

#include "ndiwrapper.h"

#pragma comment(lib, "windowsapp")


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mediaPlayer(this)
    , m_videoWidget(this)
    , m_ndiReceiver(this)
    , m_ndiSender(this)
{
    setWindowTitle(QCoreApplication::applicationName());
    QIcon icon(":/Logos/NDI_Yellow_Inverted.ico");
    setWindowIcon(icon);
    setWindowIconText(windowTitle());

    resize(960, 540);

    m_videoSink = m_videoWidget.videoSink();
    setCentralWidget(&m_videoWidget);

    createActions();
    createTrayIcon();
}

MainWindow::~MainWindow()
{
    qDebug() << "+~MainWindow()";
    captureStop();
    ndiReceiverStop();
    m_mediaPlayer.stop();
    qDebug() << "-~MainWindow()";
}

void MainWindow::createActions()
{
    m_pActionFullScreen = new QAction(tr("Full Screen Toggle"), this);
    m_pActionFullScreen->setCheckable(true);
    connect(m_pActionFullScreen, &QAction::triggered, this, &MainWindow::onActionFullScreenTriggered);

    m_pActionRestore = new QAction(QString(tr("Open %1")).arg(windowTitle()), this);
    auto font = m_pActionRestore->font();
    font.setBold(true);
    m_pActionRestore->setFont(font);
    m_pActionRestore->setIcon(windowIcon());
    connect(m_pActionRestore, &QAction::triggered, this, &MainWindow::onActionRestoreWindowTriggered);

    m_pActionExit = new QAction(tr("E&xit"), this);
    connect(m_pActionExit, &QAction::triggered, this, &MainWindow::onActionExitTriggered);
}

void MainWindow::createTrayIcon()
{
    m_pTrayIconMenu = new QMenu(this);
    m_pTrayIconMenu->addAction(m_pActionRestore);
    m_pTrayIconMenu->addSeparator();
    m_pMenuMonitors = m_pTrayIconMenu->addMenu(tr("Capture Monitor"));
    m_pTrayIconMenu->addSeparator();
    m_pTrayIconMenu->addAction(m_pActionExit);

    m_pTrayIcon = new QSystemTrayIcon(this);
    m_pTrayIcon->setContextMenu(m_pTrayIconMenu);
    m_pTrayIcon->setIcon(windowIcon());
    m_pTrayIcon->setToolTip(windowTitle());
    connect(m_pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_ndiSender.isCapturing())
    {
        // TODO: Prompt if they really want to stop capturing and close
        hide();
        event->ignore();
    }
}

void MainWindow::showEvent(QShowEvent*)
{
    qDebug() << "showEvent(...)";
    bool isFirstShowing = m_mediaPlayer.source().isEmpty();
    if (isFirstShowing)
    {
        m_mediaPlayer.setVideoSink(m_videoSink);
        m_mediaPlayer.setSource(QUrl("qrc:/Logos/NDI Loop.mp4"));
        m_mediaPlayer.setLoops(QMediaPlayer::Infinite);
    }
    m_mediaPlayer.play();
    ndiReceiverStart();
}

void MainWindow::hideEvent(QHideEvent*)
{
    qDebug() << "hideEvent(...)";
    m_mediaPlayer.stop();
    ndiReceiverStop();
}

#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menuUpdateNdiSources(&menu);
    menu.addSeparator();
    m_pActionFullScreen->setChecked(isFullScreen());
    menu.addAction(m_pActionFullScreen);
    menu.addSeparator();
    menuUpdateMonitors(m_pMenuMonitors);
    menu.addMenu(m_pMenuMonitors);
    menu.addSeparator();
    menu.addAction(m_pActionExit);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::ActivationReason::DoubleClick:
        show();
        break;
    case QSystemTrayIcon::ActivationReason::Context:
        trayIconMenuUpdate();
        break;
    default:
        // ignore
        break;
    }
}

void MainWindow::trayIconMenuUpdate()
{
    qDebug() << "+updateTrayIconMenu()";
    menuUpdateMonitors(m_pMenuMonitors);
    qDebug() << "-updateTrayIconMenu()";
}

void MainWindow::menuUpdateNdiSources(QMenu* menu)
{
    auto currentNdiSourceName = m_ndiReceiver.selectedSourceName();

    auto action = new QAction(tr("None"), this);
    action->setCheckable(true);
    if (currentNdiSourceName.isNull() || currentNdiSourceName.isEmpty())
    {
        action->setChecked(true);
    }
    connect(action, &QAction::triggered, this, &MainWindow::onActionNdiSourceTriggered);
    menu->addAction(action);

    auto ndiSources = NdiWrapper::ndiFindSources();
    uint32_t i = 0;
    for (auto it = ndiSources.begin(); it != ndiSources.end(); ++it)
    {
        auto cNdiSourceName = it.key();
        auto cNdiSourceAddress  = QString::fromUtf8(it.value().p_url_address);
        qDebug().nospace() << "[" << i++ << "] ADDRESS: " << cNdiSourceAddress << ", NAME: " << cNdiSourceName;

        auto actionText = cNdiSourceName;
        if (true)
        {
            actionText += " {address:" + cNdiSourceAddress + "}";
        }
        action = new QAction(actionText, this);
        action->setCheckable(true);
        if (currentNdiSourceName == cNdiSourceName)
        {
            action->setChecked(true);
        }
        action->setData(cNdiSourceName);
        connect(action, &QAction::triggered, this, &MainWindow::onActionNdiSourceTriggered);
        menu->addAction(action);
    }
}

bool operator < (const Monitor &lhs, const Monitor &rhs)
{
    return lhs.ClassName() < rhs.ClassName();
}

void MainWindow::menuUpdateMonitors(QMenu* menu)
{
    auto monitors = getMonitors();
    std::sort(monitors.begin(), monitors.end());

    auto selectedMonitorName = m_selectedMonitorName;
    qDebug() << "selectedMonitorName" << selectedMonitorName;

    menu->clear();

    auto action = new QAction(tr("None"), menu);
    action->setData(0);
    action->setCheckable(true);
    if (selectedMonitorName.isEmpty())
    {
        action->setChecked(true);
    }
    connect(action, &QAction::triggered, this, &MainWindow::onActionMonitorTriggered);
    menu->addAction(action);

    for (auto monitor : monitors)
    {
        auto monitorName = QString::fromStdWString(monitor.ClassName());

        auto actionText = monitorName;
        auto action = new QAction(actionText, menu);
        action->setCheckable(true);

        if (monitorName == selectedMonitorName)
        {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, this, &MainWindow::onActionMonitorTriggered);
        menu->addAction(action);
    }
}

void MainWindow::onActionNdiSourceTriggered()
{
    qDebug() << "onActionNdiSourceTriggered()";

    auto action = qobject_cast<QAction*>(sender());
    if (!action) return;
    auto actionText = action->text();

    QString selectedNdiSourceName = (actionText == tr("None")) ? nullptr : action->data().toString();
    qDebug() << "selectedNdiSourceName" << selectedNdiSourceName;

    m_ndiReceiver.selectSource(selectedNdiSourceName);
}

void MainWindow::onActionMonitorTriggered()
{
    qDebug() << "onActionMonitorTriggered()";

    auto action = qobject_cast<QAction*>(sender());
    if (!action) return;
    auto actionText = action->text();

    auto selectedMonitorName = (actionText == tr("None")) ? "" : actionText;
    qDebug() << "selectedMonitorName" << selectedMonitorName;

    m_selectedMonitorName = selectedMonitorName;

    if (selectedMonitorName.isEmpty())
    {
        captureStop();
    }
    else
    {
        captureStart();
    }
}

void MainWindow::onActionFullScreenTriggered()
{
    qDebug() << "onActionFullScreenTriggered()";
    setFullScreen(!isFullScreen());
}

void MainWindow::onActionRestoreWindowTriggered()
{
    qDebug() << "onActionRestoreWindowTriggered()";
    show();
}

void MainWindow::onActionExitTriggered()
{
    qDebug() << "onActionExitTriggered()";
    QApplication::exit(0);
}

void MainWindow::setFullScreen(bool fullScreen)
{
    qDebug().nospace() << "setFullScreen(" << fullScreen << ")";
    if (fullScreen)
    {
        setWindowState(Qt::WindowFullScreen);
    }
    else
    {
        setWindowState(Qt::WindowNoState);
    }
}

void MainWindow::ndiReceiverStart()
{
    qDebug() << "+ndiReceiverStart()";
    ndiReceiverStop();
    connect(&m_ndiReceiver, &NdiReceiver::onSourceConnected, this, &MainWindow::onNdiReceiverSourceConnected);
    connect(&m_ndiReceiver, &NdiReceiver::onMetadataReceived, this, &MainWindow::onNdiReceiverMetadataReceived);
    connect(&m_ndiReceiver, &NdiReceiver::onVideoFrameReceived, this, &MainWindow::onNdiReceiverVideoFrameReceived);
    connect(&m_ndiReceiver, &NdiReceiver::onSourceDisconnected, this, &MainWindow::onNdiReceiverSourceDisconnected);
    m_ndiReceiver.start();
    qDebug() << "-ndiReceiverStart()";
}

void MainWindow::onNdiReceiverSourceConnected(QString sourceName)
{
    qDebug() << "onNdiReceiverSourceConnected(" << sourceName << ")";
    qDebug() << "m_mediaPlayer.stop()";
    m_mediaPlayer.stop();
}

void MainWindow::onNdiReceiverMetadataReceived(QString metadata)
{
    qDebug() << "onNdiReceiverMetadataReceived(" << metadata << ")";
    //...
}

void MainWindow::onNdiReceiverVideoFrameReceived(const QVideoFrame& videoFrame)
{
    m_videoSink->setVideoFrame(videoFrame);
}

void MainWindow::onNdiReceiverSourceDisconnected(QString sourceName)
{
    qDebug() << "onNdiReceiverSourceDisconnected(" << sourceName << ")";
    qDebug() << "m_mediaPlayer.play()";
    m_mediaPlayer.play();
}

void MainWindow::ndiReceiverStop()
{
    qDebug() << "+ndiReceiverStop()";
    disconnect(&m_ndiReceiver, &NdiReceiver::onSourceConnected, this, &MainWindow::onNdiReceiverSourceConnected);
    disconnect(&m_ndiReceiver, &NdiReceiver::onMetadataReceived, this, &MainWindow::onNdiReceiverMetadataReceived);
    disconnect(&m_ndiReceiver, &NdiReceiver::onVideoFrameReceived, this, &MainWindow::onNdiReceiverVideoFrameReceived);
    disconnect(&m_ndiReceiver, &NdiReceiver::onSourceDisconnected, this, &MainWindow::onNdiReceiverSourceDisconnected);
    m_ndiReceiver.stop();
    qDebug() << "-ndiReceiverStop()";
}

//
// public methods
//

std::vector<Monitor> MainWindow::getMonitors()
{
    return EnumerateMonitors();
}

void MainWindow::captureStart(HMONITOR hmonitor)
{
    qDebug() << "+captureStart(...)";

    captureStop();

    if (hmonitor == 0)
    {
        auto monitors = getMonitors();
        foreach (auto monitor, monitors)
        {
            if (monitor.IsPrimary())
            {
                hmonitor = monitor.Hmonitor();
                break;
            }
        }
        if (hmonitor == 0)
        {
            qDebug() << "Failed to find any monitors to capture";
            return;
        }
    }

    connect(&m_ndiSender, &NdiSender::onMetadataReceived, this, &MainWindow::onNdiSenderMetadataReceived);
    connect(&m_ndiSender, &NdiSender::onReceiverCountChanged, this, &MainWindow::onNdiSenderReceiverCountChanged);

    m_ndiSender.start(hmonitor);

    // Never hide this icon once it is shown
    m_pTrayIcon->show();

    qDebug() << "-captureStart(...)";
}

void MainWindow::captureStop()
{
    qDebug() << "+captureStop()";
    disconnect(&m_ndiSender, &NdiSender::onMetadataReceived, this, &MainWindow::onNdiSenderMetadataReceived);
    disconnect(&m_ndiSender, &NdiSender::onReceiverCountChanged, this, &MainWindow::onNdiSenderReceiverCountChanged);
    m_ndiSender.stop();
    qDebug() << "-captureStop()";
}

void MainWindow::onNdiSenderMetadataReceived(QString metadata)
{
    qDebug() << "onNdiSenderMetadataReceived(" << metadata << ")";
    //...
}

void MainWindow::onNdiSenderReceiverCountChanged(int receiverCount)
{
    qDebug() << "onReceiverCountChanged(" << receiverCount << ")";
    //...
}
