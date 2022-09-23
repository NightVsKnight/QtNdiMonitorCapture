#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
#include <QSystemTrayIcon>
#include <QtMultimediaWidgets/QVideoWidget>

#include "Win32MonitorEnumeration.h"

#include "ndireceiver.h"
#include "ndisender.h"

#define NUM_CAPTURE_FRAME_BUFFERS 2

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    std::vector<Monitor> getMonitors();
    void captureStart(HMONITOR hmonitor = 0);
    void captureStop();

private:
    QAction* m_pActionFullScreen;
    QAction* m_pActionRestore;
    QAction* m_pActionExit;
    QMenu* m_pMenuMonitors;
    QMenu* m_pTrayIconMenu;
    QSystemTrayIcon* m_pTrayIcon;
    void createActions();
    void createTrayIcon();

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent* event) override;
#endif // QT_NO_CONTEXTMENU
private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
private:
    void trayIconMenuUpdate();
    void menuUpdateNdiSources(QMenu* menu);
    void menuUpdateMonitors(QMenu* menu);

private slots:
    void onActionNdiSourceTriggered();
    void onActionMonitorTriggered();
    void onActionFullScreenTriggered();
    void onActionRestoreWindowTriggered();
    void onActionExitTriggered();

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
    void onNdiReceiverMetadataReceived(QString metadata);
    void onNdiReceiverSourceConnected(QString sourceName);
    void onNdiReceiverSourceDisconnected(QString sourceName);

private:
    QString m_selectedMonitorName;
    NdiSender m_ndiSender;
private slots:
    void onNdiSenderMetadataReceived(QString metadata);
    void onNdiSenderReceiverCountChanged(int receiverCount);
};
