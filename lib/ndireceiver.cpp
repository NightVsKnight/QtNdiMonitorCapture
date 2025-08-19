#include "ndireceiver.h"

#include <QDebug>
#include <QAudioDevice>

NdiReceiver::NdiReceiver(QObject *parent)
    : QObject(parent)
{
    init();
}

NdiReceiver::~NdiReceiver()
{
    qDebug() << "+~NdiReceiver()";
    stop();
    qDebug() << "-~NdiReceiver()";
}

void NdiReceiver::init()
{
    qDebug() << "+init()";

    m_threadNdiReceiver.setObjectName("threadNdiReceiver");
    m_workerNdiReceiver.moveToThread(&m_threadNdiReceiver);

    connect(&m_threadNdiReceiver, &QThread::started, &m_workerNdiReceiver, &NdiReceiverWorker::run);
    connect(&m_threadNdiReceiver, &QThread::finished, &m_workerNdiReceiver, &NdiReceiverWorker::stop);

    connect(&m_workerNdiReceiver, &NdiReceiverWorker::onMetadataReceived, this, &NdiReceiver::onMetadataReceived);
    connect(&m_workerNdiReceiver, &NdiReceiverWorker::onSourceConnected, this, &NdiReceiver::onSourceConnected);
    connect(&m_workerNdiReceiver, &NdiReceiverWorker::onVideoFrameReceived, this, &NdiReceiver::onVideoFrameReceived);
    connect(&m_workerNdiReceiver, &NdiReceiverWorker::onSourceDisconnected, this, &NdiReceiver::onSourceDisconnected);
    qRegisterMetaType<QAudioDevice>("QAudioDevice");
    connect(this, &NdiReceiver::audioOutputDeviceChanged, &m_workerNdiReceiver, &NdiReceiverWorker::setAudioOutputDevice);

    qDebug() << "-init()";
}

void NdiReceiver::start()
{
    qDebug() << "+start(...)";
    m_threadNdiReceiver.start();
    qDebug() << "-start(...)";
}

void NdiReceiver::stop()
{
    qDebug() << "+stop()";
    m_workerNdiReceiver.stop();
    m_threadNdiReceiver.quit();
    m_threadNdiReceiver.wait();
    qDebug() << "-stop()";
}

void NdiReceiver::setConnectionInfo(const QString& receiverName, const QString& connectionMetadata)
{
    m_workerNdiReceiver.setConnectionInfo(receiverName, connectionMetadata);
}

QString NdiReceiver::selectedSourceName()
{
    return m_workerNdiReceiver.selectedSourceName();
}

void NdiReceiver::selectSource(const QString& sourceName)
{
    m_workerNdiReceiver.selectSource(sourceName);
}

void NdiReceiver::sendMetadata(const QString& metadata)
{
    m_workerNdiReceiver.sendMetadata(metadata);
}

void NdiReceiver::muteAudio(bool bMute)
{
    m_workerNdiReceiver.muteAudio(bMute);
}

void NdiReceiver::setAudioOutputDevice(const QAudioDevice& device)
{
    emit audioOutputDeviceChanged(device);
}
