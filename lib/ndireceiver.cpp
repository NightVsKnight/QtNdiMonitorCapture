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
    threadNdiReceiver.setObjectName("threadNdiReceiver");
    workerNdiReceiver.moveToThread(&threadNdiReceiver);
    connect(&threadNdiReceiver, &QThread::started, &workerNdiReceiver, &NdiReceiverWorker::process);
    connect(&threadNdiReceiver, &QThread::finished, &workerNdiReceiver, &NdiReceiverWorker::stop);
    qDebug() << "-init()";
}

NdiReceiverWorker &NdiReceiver::getWorker()
{
    return workerNdiReceiver;
}

/*
void NdiReceiver::addVideoSink(QVideoSink *videoSink)
{
    workerNdiReceiver.addVideoSink(videoSink);
}

void NdiReceiver::removeVideoSink(QVideoSink *videoSink)
{
    workerNdiReceiver.removeVideoSink(videoSink);
}

QString NdiReceiver::getNdiSourceName()
{
    return workerNdiReceiver.getNdiSourceName();
}

void NdiReceiver::setNdiSourceName(QString ndiSourceName)
{
    workerNdiReceiver.setNdiSourceName(ndiSourceName);
}
*/

void NdiReceiver::start(QVideoSink *videoSink)
{
    qDebug() << "+start(...)";
    workerNdiReceiver.addVideoSink(videoSink);
    threadNdiReceiver.start();
    qDebug() << "-start(...)";
}

void NdiReceiver::stop()
{
    qDebug() << "+stop()";
    workerNdiReceiver.stop();
    threadNdiReceiver.quit();
    threadNdiReceiver.wait();
    qDebug() << "-stop()";
}
