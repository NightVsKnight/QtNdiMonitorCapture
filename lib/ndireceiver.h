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

    /*
    void addVideoSink(QVideoSink *videoSink);
    void removeVideoSink(QVideoSink *videoSink);
    QString getNdiSourceName();
    void setNdiSourceName(QString ndiSourceName);
    */
    void start(QVideoSink *videoSink = nullptr);
    void stop();

signals:
    //...

private:
    NdiReceiverWorker workerNdiReceiver;
    QThread threadNdiReceiver;

    void init();
};

#endif // NDIRECEIVER_H
