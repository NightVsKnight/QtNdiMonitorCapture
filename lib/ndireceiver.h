#ifndef NDIRECEIVER_H
#define NDIRECEIVER_H

#include <QObject>
#include <QSharedDataPointer>
#include <QThread>

#include "ndireceiverworker.h"

class NdiReceiver : public QObject
{
    Q_OBJECT
public:
    NdiReceiver(QObject *parent = nullptr);
    ~NdiReceiver();

    void start(QVideoSink *videoSink = nullptr);
    void stop();

    void setConnectionInfo(QString receiverName, QString connectionMetadata)
    {
        m_workerNdiReceiver.setConnectionInfo(receiverName, connectionMetadata);
    }
    void addVideoSink(QVideoSink *videoSink)
    {
        m_workerNdiReceiver.addVideoSink(videoSink);
    }
    void removeVideoSink(QVideoSink *videoSink)
    {
        m_workerNdiReceiver.removeVideoSink(videoSink);
    }
    QString selectedSourceName()
    {
        return m_workerNdiReceiver.selectedSourceName();
    }
    void selectSource(QString sourceName)
    {
        m_workerNdiReceiver.selectSource(sourceName);
    }
    void sendMetadata(QString metadata)
    {
        m_workerNdiReceiver.sendMetadata(metadata);
    }
    void muteAudio(bool bMute)
    {
        m_workerNdiReceiver.muteAudio(bMute);
    }

signals:
    void onMetadataReceived(QString metadata);
    void onSourceConnected(QString sourceName);
    void onSourceDisconnected(QString sourceName);

private:
    NdiReceiverWorker m_workerNdiReceiver;
    QThread m_threadNdiReceiver;

    void init();
};

#endif // NDIRECEIVER_H
