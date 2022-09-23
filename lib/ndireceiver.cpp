#include "ndireceiver.h"

#include <QDebug>

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
    connect(&m_workerNdiReceiver, &NdiReceiverWorker::onSourceDisconnected, this, &NdiReceiver::onSourceDisconnected);

    qDebug() << "-init()";
}

void NdiReceiver::start(QVideoSink *videoSink)
{
    qDebug() << "+start(...)";
    m_workerNdiReceiver.addVideoSink(videoSink);
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
