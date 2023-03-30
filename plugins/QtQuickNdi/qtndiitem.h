#ifndef QTNDIITEM_H
#define QTNDIITEM_H

#include <QQuickPaintedItem >
#include <QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>

#include "ndireceiver.h"

class QtNdiItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ndiSource READ ndiSource WRITE setNdiSource NOTIFY ndiSourceChanged)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
    QML_ELEMENT
    Q_DISABLE_COPY(QtNdiItem)
public:
    explicit QtNdiItem(QQuickItem *parent = nullptr);
    ~QtNdiItem() override;

    QString ndiSource() const;
    void setNdiSource(const QString &name);

    void setVideoOutput(QObject *);
    QObject *videoOutput() const;

    bool active() const;
    void setActive(bool active);

    bool mute() const;
    void setMute(bool active);

protected:

signals:
    void ndiSourceChanged(const QString ndiSource);
    void videoOutputChanged(QObject* videoOutput);
    void activeChanged(bool active);
    void muteChanged(bool mute);
    void ndiSourceConnected(const QString ndiSource);
    void ndiSourceDisconnected(const QString ndiSource);
    void ndiSourceMetadataReceived(const QString metadata);

private:
    QString m_ndiSource;
    QVideoSink *m_videoSink = nullptr;
    QPointer<QObject> m_videoOutput;
    NdiReceiver m_ndiReceiver;
    bool m_active;
    bool m_mute;
    void ndiReceiverStart();
    void ndiReceiverStop();
    void ndiReceiverStateUpdate();
private slots:
    void onNdiReceiverSourceConnected(QString sourceName);
    void onNdiReceiverMetadataReceived(QString metadata);
    void onNdiReceiverVideoFrameReceived(const QVideoFrame& videoFrame);
    void onNdiReceiverSourceDisconnected(QString sourceName);

};

#endif // QTNDIITEM_H
