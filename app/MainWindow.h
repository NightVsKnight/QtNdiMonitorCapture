#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMetaEnum>
#include <QScreenCapture>
#include <QSystemTrayIcon>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QAudioDevice>

#include "ndireceiver.h"
#include "ndisender.h"

#define NUM_CAPTURE_FRAME_BUFFERS 2

namespace App
{
    Q_NAMESPACE

    enum class AppMode
    {
        Monitor,
        Capture
    };
    Q_ENUM_NS(AppMode)

    static QString toString(AppMode mode)
    {
        return QMetaEnum::fromType<AppMode>().valueToKey((int)mode);
    }

    static QString toCamelCase(const QString& s)
    {
        QStringList cased;
        foreach (auto word, s.split(" ", Qt::SkipEmptyParts))
        {
            cased << word.at(0).toUpper() + word.mid(1);
        }
        return cased.join(' ');
    }

    static AppMode toAppMode(const QString& mode, bool* ok)
    {
        auto b = toCamelCase(mode).toUtf8();
        return (AppMode)QMetaEnum::fromType<AppMode>().keyToValue(b.data(), ok);
    }

    static QStringList getAppModes()
    {
        QStringList modes;
        auto meta = QMetaEnum::fromType<AppMode>();
        auto keyCount = meta.keyCount();
        for (int i = 0; i < keyCount; ++i)
        {
            auto key = meta.key(i);
            modes.append(key);
        }
        return modes;
    }
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QList<QScreen*> getCaptureScreens();
    /**
     * @brief captureStart
     * @param screen screen to use; if nullptr then use the primary screen
     */
    void captureStart(QScreen* screen = nullptr);
    void captureStop();

private:
    QAction* m_pActionFullScreen;
    QAction* m_pActionRestore;
    QAction* m_pActionExit;
    QMenu* m_pMenuCaptureScreens;
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
    void menuUpdateCaptureScreens(QMenu* menu);

private slots:
    void onActionNdiSourceTriggered();
    void onActionCaptureScreenTriggered();
    void onActionFullScreenTriggered();
    void onActionRestoreWindowTriggered();
    void onActionExitTriggered();
    void onActionAudioOutputDeviceTriggered();

private:
    void setFullScreen(bool fullScreen);

private:
    QScreenCapture *captureScreen = nullptr;
    QVideoSink *captureSink = nullptr;
    QMediaCaptureSession *mediaCaptureSession = nullptr;
    QMediaPlayer m_mediaPlayer;
    QVideoWidget m_videoWidget;
    QVideoSink* m_videoSink;
    NdiReceiver m_ndiReceiver;
    void ndiReceiverStart();
    void ndiReceiverStop();
private slots:
    void onNdiReceiverSourceConnected(QString sourceName);
    void onNdiReceiverMetadataReceived(QString metadata);
    void onNdiReceiverVideoFrameReceived(const QVideoFrame& videoFrame);
    void onNdiReceiverSourceDisconnected(QString sourceName);

private:
    QString m_selectedCaptureScreenName;
    NdiSender m_ndiSender;
    QAudioDevice m_selectedAudioOutputDevice;
private slots:
    void onMediaCaptureVideoFrame(const QVideoFrame &frame);
    void onNdiSenderMetadataReceived(QString metadata);
    void onNdiSenderReceiverCountChanged(int receiverCount);
};
