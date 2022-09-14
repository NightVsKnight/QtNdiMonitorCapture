#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
#include <QSystemTrayIcon>
#include <QtMultimediaWidgets/QVideoWidget>

#include "SimpleCapture.h"
#include "Win32MonitorEnumeration.h"

#include "ndireceiver.h"

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::UI::Composition;

#define NUM_CAPTURE_FRAME_BUFFERS 2

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    std::vector<Monitor> getMonitors();
    void captureStart(HMONITOR hmonitor = 0);
    void captureStop();

private:
    QAction *actionFullScreenToggle;
    QAction *actionCaptureToggle;
    QAction *actionExit;
    QAction *actionRestore;
    void createActions();

    QMenu *trayIconMenu;
    QSystemTrayIcon *trayIcon;
    void createTrayIcon();

    void updateActionCaptureToggle();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU

private:
    void addNdiSources(QMenu *menu);

private slots:
    void onActionNdiSourceSelected();
    void onActionFullScreenToggle();
    void onActionCaptureToggle();
    void onActionExit();
    void onActionRestoreWindow();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private slots:
    void onMonitorSelected(int index);

private:
    void setFullScreen(bool fullScreen);

private:
    bool m_hasShownAtLeastOnce;
    QMediaPlayer m_mediaPlayer;
    QVideoWidget m_videoWidget;
    NdiReceiver m_ndiReceiver;
    void ndiReceiverStart();
    void ndiReceiverStop();
private slots:
    void onNdiRecieverConnected();
    void onNdiRecieverDisconnected();

private:
    Windows::System::DispatcherQueueController m_dispatcherQueueController;
    Windows::System::DispatcherQueue m_dispatcherQueue;
    atomic<SimpleCapture*> m_pSimpleCapture;
    atomic<NDIlib_send_instance_t> m_pNdiSend;
    uint64_t m_frameCount;
    size_t m_frameSizeBytes;
    uint8_t *m_pNdiSendBuffers[NUM_CAPTURE_FRAME_BUFFERS];
};
