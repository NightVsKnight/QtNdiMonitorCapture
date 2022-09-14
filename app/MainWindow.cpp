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

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.Advanced.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.Advanced.x86.lib")
#endif // _WIN64
#endif

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::Graphics::Capture;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_hasShownAtLeastOnce{false}
    , m_mediaPlayer(this)
    , m_videoWidget(this)
    , m_ndiReceiver(this)
    , m_dispatcherQueueController{nullptr}
    , m_dispatcherQueue{nullptr}
    , m_pSimpleCapture{nullptr}
    , m_pNdiSend{nullptr}
    , m_frameCount{0}
    , m_frameSizeBytes{0}
    , m_pNdiSendBuffers{{}}
{
    setWindowTitle(QCoreApplication::applicationName());
    QIcon icon(":/Logos/NDI_Yellow_Inverted.ico");
    setWindowIcon(icon);
    setWindowIconText(windowTitle());

    resize(960, 540);

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
    actionFullScreenToggle = new QAction(tr("Full Screen Toggle"), this);
    connect(actionFullScreenToggle, &QAction::triggered, this, &MainWindow::onActionFullScreenToggle);

    actionCaptureToggle = new QAction(tr("Monitor Capture Start"), this);
    connect(actionCaptureToggle, &QAction::triggered, this, &MainWindow::onActionCaptureToggle);

    actionExit = new QAction(tr("E&xit"), this);
    connect(actionExit, &QAction::triggered, this, &MainWindow::onActionExit);

    actionRestore = new QAction(QString(tr("Open %1")).arg(windowTitle()), this);
    auto font = actionRestore->font();
    font.setBold(true);
    actionRestore->setFont(font);
    actionRestore->setIcon(windowIcon());
    connect(actionRestore, &QAction::triggered, this, &MainWindow::onActionRestoreWindow);
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(actionRestore);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(actionCaptureToggle);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(actionExit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(windowIcon());
    trayIcon->setToolTip(windowTitle());
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
}

void MainWindow::updateActionCaptureToggle()
{
    if (m_pSimpleCapture)
    {
        actionCaptureToggle->setText(tr("Monitor Capture Stop"));// %1", XYZ);
    }
    else
    {
        actionCaptureToggle->setText(tr("Monitor Capture Start"));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_pSimpleCapture)
    {
        hide();
        event->ignore();
    }
}

void MainWindow::showEvent(QShowEvent *)
{
    qDebug() << "showEvent(...)";
    if (!m_hasShownAtLeastOnce)
    {
        m_hasShownAtLeastOnce = true;
        m_mediaPlayer.setVideoSink(m_videoWidget.videoSink());
        m_mediaPlayer.setSource(QUrl("qrc:/Logos/NDI Loop.mp4"));
        m_mediaPlayer.setLoops(QMediaPlayer::Infinite);
    }
    m_mediaPlayer.play();
    ndiReceiverStart();
}

void MainWindow::hideEvent(QHideEvent *)
{
    qDebug() << "hideEvent(...)";
    m_mediaPlayer.stop();
    ndiReceiverStop();
}

#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    addNdiSources(&menu);
    menu.addSeparator();
    menu.addAction(actionFullScreenToggle);
    menu.addSeparator();
    updateActionCaptureToggle();
    menu.addAction(actionCaptureToggle);
    menu.addSeparator();
    menu.addAction(actionExit);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void MainWindow::addNdiSources(QMenu *menu)
{
    auto currentNdiSourceName = m_ndiReceiver.getWorker().getNdiSourceName();
    auto action = new QAction(tr("None"), this);
    action->setCheckable(true);
    if (currentNdiSourceName.isNull() || currentNdiSourceName.isEmpty())
    {
        action->setChecked(true);
    }
    connect(action, &QAction::triggered, this, &MainWindow::onActionNdiSourceSelected);
    menu->addAction(action);
    auto ndiSources = NdiWrapper::get().ndiFindSources();
    uint32_t i = 0;
    for (QMap<QString, NDIlib_source_t>::iterator it = ndiSources.begin(); it != ndiSources.end(); ++it) {
        QString cNdiSourceName = it.key();
        QString cNdiSourceAddress  = QString::fromUtf8(it.value().p_url_address);
        qDebug().nospace() << "[" << i++ << "] ADDRESS: " << cNdiSourceAddress << ", NAME: " << cNdiSourceName;

        QString actionText = cNdiSourceName;
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
        connect(action, &QAction::triggered, this, &MainWindow::onActionNdiSourceSelected);
        menu->addAction(action);
    }
}

void MainWindow::onActionNdiSourceSelected()
{
    auto action = qobject_cast<QAction*>(sender());
    if (!action) return;
    QString cNdiSourceName = (action->text() == tr("None")) ? nullptr : action->data().toString();
    qDebug() << "cNdiSourceName" << cNdiSourceName;
    m_ndiReceiver.getWorker().setNdiSourceName(cNdiSourceName);
}

void MainWindow::onActionFullScreenToggle()
{
    qDebug() << "onActionFullScreenToggle()";
    setFullScreen(windowState() != Qt::WindowFullScreen);
}

void MainWindow::onActionCaptureToggle()
{
    auto action = qobject_cast<QAction*>(sender());
    if (!action) return;
    auto hmonitor = (HMONITOR)action->data().toULongLong();
    if (m_pSimpleCapture)
    {
        captureStop();
    }
    else
    {
        captureStart(hmonitor);
    }
}

void MainWindow::onActionRestoreWindow()
{
    show();
}

void MainWindow::onActionExit()
{
    qDebug() << "onActionExit()";
    QApplication::exit(0);
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::ActivationReason::DoubleClick:
        show();
        break;
    case QSystemTrayIcon::ActivationReason::Context:
    {
        qDebug() << "onTrayIconActivated QSystemTrayIcon::ActivationReason::Context";
        updateActionCaptureToggle();
        break;
    }
    default:
        // ignore
        break;
    }
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
    connect(&m_ndiReceiver.getWorker(), &NdiReceiverWorker::ndiSourceConnected, this, &MainWindow::onNdiRecieverConnected);
    connect(&m_ndiReceiver.getWorker(), &NdiReceiverWorker::ndiSourceDisconnected, this, &MainWindow::onNdiRecieverDisconnected);
    m_ndiReceiver.start(m_videoWidget.videoSink());
    qDebug() << "-ndiReceiverStart()";
}

void MainWindow::onNdiRecieverConnected()
{
    qDebug() << "onNdiRecieverConnected()";
    qDebug() << "m_mediaPlayer.stop()";
    m_mediaPlayer.stop();
}

void MainWindow::onNdiRecieverDisconnected()
{
    qDebug() << "onNdiRecieverDisconnected()";
    qDebug() << "m_mediaPlayer.play()";
    m_mediaPlayer.play();
}

void MainWindow::ndiReceiverStop()
{
    qDebug() << "+ndiReceiverStop()";
    disconnect(&m_ndiReceiver.getWorker(), &NdiReceiverWorker::ndiSourceConnected, this, &MainWindow::onNdiRecieverConnected);
    disconnect(&m_ndiReceiver.getWorker(), &NdiReceiverWorker::ndiSourceDisconnected, this, &MainWindow::onNdiRecieverDisconnected);
    m_ndiReceiver.stop();
    qDebug() << "-ndiReceiverStop()";
}

std::vector<Monitor> MainWindow::getMonitors()
{
    return EnumerateMonitors();
}

void MainWindow::onMonitorSelected(int index)
{
    if (index == 0) return;
    auto monitors = getMonitors();
    auto monitor = monitors[index - 1];
    captureStart(monitor.Hmonitor());
}

// Direct3D11CaptureFramePool [in SimpleCapture] requires a DispatcherQueue
auto CreateDispatcherQueueController()
{
    namespace abi = ABI::Windows::System;
    DispatcherQueueOptions options
    {
        sizeof(DispatcherQueueOptions),
        DQTYPE_THREAD_CURRENT,
        DQTAT_COM_STA
    };
    Windows::System::DispatcherQueueController controller{ nullptr };
    check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(put_abi(controller))));
    return controller;
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

    if (m_dispatcherQueueController == nullptr)
    {
        m_dispatcherQueueController = CreateDispatcherQueueController();
        m_dispatcherQueue = m_dispatcherQueueController.DispatcherQueue();
    }

    // Enqueue our capture work on the dispatcher
    auto success = m_dispatcherQueue.TryEnqueue(Windows::System::DispatcherQueuePriority::High,
    [this, hmonitor]()
    {
        auto item = CreateCaptureItemForMonitor(hmonitor);

        // TODO: Make this work for multiple pixel formats and then allow this to be passed in as a parameter
        // Must be compatible with NDI_video_frame.FourCC [below]
        auto pixelFormat = DirectXPixelFormat::B8G8R8A8UIntNormalized;
        uint pixelSizeBytes = 4; // TODO: auto-calculate this byte size based on pixelFormat

        m_pSimpleCapture = new SimpleCapture();
        (*m_pSimpleCapture).StartCapture(item, pixelFormat, pixelSizeBytes, NUM_CAPTURE_FRAME_BUFFERS,
        [this](SimpleCapture*, Direct3D11CaptureFrame frame) -> bool
        {
            auto pNdiSend = m_pNdiSend.load();
            if (!pNdiSend || NDIlib_send_get_no_connections(pNdiSend, 0) == 0)
                return false;
            if (frame == nullptr)
            {
                // End of capture
                NDIlib_send_send_video_async_v2(pNdiSend, NULL);
            }
            return true;
        },
        [this](SimpleCapture*, uint frameWidth, uint frameHeight, uint frameStrideBytes, void* pFrameBuffer)
        {
            auto pNdiSend = m_pNdiSend.load();
            if (!pNdiSend || !pFrameBuffer)
                return;

            auto thisFrameSizeBytes = frameStrideBytes * frameHeight;
            if (m_frameSizeBytes < thisFrameSizeBytes)
            {
                qDebug() << "growing m_pNdiSendBuffers from" << m_frameSizeBytes << "to" << thisFrameSizeBytes << "bytes";
                m_frameSizeBytes = thisFrameSizeBytes;
                for (int i = 0; i < NUM_CAPTURE_FRAME_BUFFERS; ++i)
                {
                    if (m_pNdiSendBuffers[i])
                    {
                        delete[] m_pNdiSendBuffers[i];
                    }
                    m_pNdiSendBuffers[i] = new uint8_t[thisFrameSizeBytes];
                }
            }

            uint8_t *pOutBuffer = m_pNdiSendBuffers[m_frameCount++ % NUM_CAPTURE_FRAME_BUFFERS];
            memcpy(pOutBuffer, pFrameBuffer, thisFrameSizeBytes);

            NDIlib_video_frame_v2_t NDI_video_frame;
            NDI_video_frame.xres = frameWidth;
            NDI_video_frame.yres = frameHeight;
            // Must be compatible with pixelFormat [above]
            NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRA;
            NDI_video_frame.line_stride_in_bytes = frameStrideBytes;
            NDI_video_frame.p_data = pOutBuffer;

            NDIlib_send_send_video_async_v2(pNdiSend, &NDI_video_frame);
        });
    });
    WINRT_VERIFY(success);

    NDIlib_send_create_t NDI_send_create_desc;
    NDI_send_create_desc.p_ndi_name = "ScreenCaptureForHMONITOR";
    m_pNdiSend = NDIlib_send_create(&NDI_send_create_desc);
    Q_ASSERT(m_pNdiSend);

    // TODO: Capture and send Audio in dedicated thread

    // Never hide this icon once it is shown
    trayIcon->show();

    qDebug() << "-captureStart(...)";
}

void MainWindow::captureStop()
{
    qDebug() << "+captureStop()";
    auto pSimpleCapture = m_pSimpleCapture.exchange(nullptr);
    if (pSimpleCapture)
    {
        pSimpleCapture->Close();
        pSimpleCapture = nullptr;
    }
    auto pNdiSend = m_pNdiSend.exchange(nullptr);
    if (pNdiSend)
    {
        NDIlib_send_send_video_async_v2(pNdiSend, NULL);
        NDIlib_send_destroy(pNdiSend);
        pNdiSend = nullptr;
    }
    for (int i = 0; i < NUM_CAPTURE_FRAME_BUFFERS; ++i)
    {
        if (m_pNdiSendBuffers[i])
        {
            delete[] m_pNdiSendBuffers[i];
            m_pNdiSendBuffers[i] = nullptr;
        }
    }
    m_frameCount = 0;
    m_frameSizeBytes = 0;
    qDebug() << "-captureStop()";
}
