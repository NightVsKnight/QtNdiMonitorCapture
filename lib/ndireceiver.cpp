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
    connect(&m_threadNdiReceiver, &QThread::started, &m_workerNdiReceiver, &NdiReceiverWorker::process);
    connect(&m_threadNdiReceiver, &QThread::finished, &m_workerNdiReceiver, &NdiReceiverWorker::stop);
    qDebug() << "-init()";
}

NdiReceiverWorker &NdiReceiver::getWorker()
{
    return m_workerNdiReceiver;
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
