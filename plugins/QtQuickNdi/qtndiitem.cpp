#include "qtndiitem.h"

#include <QVideoFrame>

QtNdiItem::QtNdiItem(QQuickItem *parent)
    : QObject(parent)
    , m_ndiSource(QStringLiteral("None"))
    , m_ndiReceiver(this)
    , m_active(true)
    , m_mute(false)
{
}

QtNdiItem::~QtNdiItem() {
    qDebug() << "+~QtNdiItem()";
    this->setNdiSource(tr("None"));
    qDebug() << "+~QtNdiItem()";
}

QString QtNdiItem::ndiSource() const {
    return this->m_ndiSource;
}

void QtNdiItem::setNdiSource(const QString &name) {
    if (this->m_ndiSource != name) {
        QString selectedNdiSourceName = (name == tr("None")) ? nullptr : name;
        qDebug() << "selectedNdiSourceName" << selectedNdiSourceName;
        m_ndiReceiver.selectSource(selectedNdiSourceName);
        this->m_ndiSource = name;

        emit this->ndiSourceChanged(this->m_ndiSource);

        this->ndiReceiverStateUpdate();
    }
}

QObject* QtNdiItem::videoOutput() const {
    return this->m_videoOutput;
}

void QtNdiItem::setVideoOutput(QObject* videoOutput) {
    if (this->m_videoOutput != videoOutput) {
        auto *sink = qobject_cast<QVideoSink *>(videoOutput);
        if (!sink && videoOutput) {
            auto *mo = videoOutput->metaObject();
            mo->invokeMethod(videoOutput, "videoSink", Q_RETURN_ARG(QVideoSink *, sink));
        }
        this->m_videoOutput = videoOutput;
        this->m_videoSink = sink;

        emit this->videoOutputChanged(this->m_videoOutput);

        this->ndiReceiverStateUpdate();
    }
}

bool QtNdiItem::active() const {
    return this->m_active;
}

void QtNdiItem::setActive(bool active) {
    if (this->m_active != active) {
        this->m_active = active;
        this->ndiReceiverStateUpdate();
        emit this->activeChanged(this->m_active);
    }
}

bool QtNdiItem::mute() const {
    return this->m_mute;
}

void QtNdiItem::setMute(bool mute) {
    if (this->m_mute != mute) {
        this->m_mute = mute;
        this->m_ndiReceiver.muteAudio(this->m_mute);
        emit this->muteChanged(this->m_mute);
    }
}

void QtNdiItem::ndiReceiverStateUpdate() {
    if (this->m_active && !this->m_ndiReceiver.selectedSourceName().isNull() && this->m_videoOutput) {
        this->ndiReceiverStart();
    }
    else {
        this->ndiReceiverStop();
    }
}

void QtNdiItem::ndiReceiverStart()
{
    qDebug() << "+ndiReceiverStart()";
    ndiReceiverStop();
    connect(&m_ndiReceiver, &NdiReceiver::onSourceConnected, this, &QtNdiItem::onNdiReceiverSourceConnected);
    connect(&m_ndiReceiver, &NdiReceiver::onMetadataReceived, this, &QtNdiItem::onNdiReceiverMetadataReceived);
    connect(&m_ndiReceiver, &NdiReceiver::onVideoFrameReceived, this, &QtNdiItem::onNdiReceiverVideoFrameReceived);
    connect(&m_ndiReceiver, &NdiReceiver::onSourceDisconnected, this, &QtNdiItem::onNdiReceiverSourceDisconnected);
    m_ndiReceiver.start();
    qDebug() << "-ndiReceiverStart()";
}

void QtNdiItem::onNdiReceiverSourceConnected(QString sourceName)
{
    qDebug() << "onNdiReceiverSourceConnected(" << sourceName << ")";
    emit this->ndiSourceConnected(sourceName);
 }

void QtNdiItem::onNdiReceiverMetadataReceived(QString metadata)
{
    qDebug() << "onNdiReceiverMetadataReceived(" << metadata << ")";
    emit this->ndiSourceMetadataReceived(metadata);
}

void QtNdiItem::onNdiReceiverVideoFrameReceived(const QVideoFrame& videoFrame)
{
    if (this->m_videoSink) {
        this->m_videoSink->setVideoFrame(videoFrame);
    }
}

void QtNdiItem::onNdiReceiverSourceDisconnected(QString sourceName)
{
    qDebug() << "onNdiReceiverSourceDisconnected(" << sourceName << ")";
    emit this->ndiSourceConnected(sourceName);
}

void QtNdiItem::ndiReceiverStop()
{
    qDebug() << "+ndiReceiverStop()";
    disconnect(&m_ndiReceiver, &NdiReceiver::onSourceConnected, this, &QtNdiItem::onNdiReceiverSourceConnected);
    disconnect(&m_ndiReceiver, &NdiReceiver::onMetadataReceived, this, &QtNdiItem::onNdiReceiverMetadataReceived);
    disconnect(&m_ndiReceiver, &NdiReceiver::onVideoFrameReceived, this, &QtNdiItem::onNdiReceiverVideoFrameReceived);
    disconnect(&m_ndiReceiver, &NdiReceiver::onSourceDisconnected, this, &QtNdiItem::onNdiReceiverSourceDisconnected);
    m_ndiReceiver.stop();
    qDebug() << "-ndiReceiverStop()";
}
