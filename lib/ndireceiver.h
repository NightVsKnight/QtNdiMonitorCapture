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

    NdiReceiverWorker &getWorker();

    void start(QVideoSink *videoSink = nullptr);
    void stop();

signals:
    //...

private:
    NdiReceiverWorker m_workerNdiReceiver;
    QThread m_threadNdiReceiver;

    void init();
};

#endif // NDIRECEIVER_H
