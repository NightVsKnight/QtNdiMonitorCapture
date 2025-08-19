#ifndef NDIRECEIVER_H
#define NDIRECEIVER_H

#include <QObject>
#include <QSharedDataPointer>
#include <QThread>
#include <QAudioDevice>

#include "ndireceiverworker.h"

class NdiReceiver : public QObject
{
    Q_OBJECT
public:
    NdiReceiver(QObject *parent = nullptr);
    ~NdiReceiver();

    void start();
    void stop();

    void setConnectionInfo(const QString& receiverName, const QString& connectionMetadata);
    QString selectedSourceName();
    void selectSource(const QString& sourceName);
    void sendMetadata(const QString& metadata);
    void muteAudio(bool bMute);
    void setAudioOutputDevice(const QAudioDevice& device);

signals:
    void onSourceConnected(const QString& sourceName);
    void onMetadataReceived(const QString& metadata);
    void onVideoFrameReceived(const QVideoFrame& videoFrame);
    void onSourceDisconnected(const QString& sourceName);
    void audioOutputDeviceChanged(const QAudioDevice& device);

private:
    NdiReceiverWorker m_workerNdiReceiver;
    QThread m_threadNdiReceiver;

    void init();
};

#endif // NDIRECEIVER_H
