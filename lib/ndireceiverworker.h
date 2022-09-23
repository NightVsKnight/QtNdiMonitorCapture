#ifndef NDIRECEIVERWORKER_H
#define NDIRECEIVERWORKER_H

#include <QObject>
#include <QAudioFormat>
#include <QAudioSink>
#include <QList>
#include <QMediaDevices>
#include <QVideoFrame>
#include <QVideoSink>

#include "Processing.NDI.Advanced.h"

#define MAX_AUDIO_LEVELS 16
#define AUDIO_LEVEL_MIN -60

class NdiReceiverWorker : public QObject
{
    Q_OBJECT
public:
    explicit NdiReceiverWorker(QObject* pParent = nullptr);
    ~NdiReceiverWorker();

    void setConnectionInfo(QString receiverName, QString connectionMetadata);

    void addVideoSink(QVideoSink* pVideoSink);
    void removeVideoSink(QVideoSink* pVideoSink);
    QString selectedSourceName();
    void selectSource(QString sourceName);

    void muteAudio(bool bMute);

    void sendMetadata(QString metadata);

signals:
    void onMetadataReceived(QString metadata);
    void onSourceConnected(QString sourceName);
    void onSourceDisconnected(QString sourceName);

public slots:
    void run();
    void stop();

private:
    bool          m_bReconnect;
    QString       m_receiverName;
    QString       m_connectionMetadata;

    QList<QVideoSink*> m_videoSinks;

    volatile bool m_bIsRunning;

    QList<QString> m_listMetadatasToSend;

    QString       m_selectedSourceName;
    volatile bool m_bMuteAudio;
    volatile bool m_bLowQuality;
    QString       m_cIDX;

    float m_fAudioLevels[MAX_AUDIO_LEVELS];

    void init();
    void processVideo(
            NDIlib_video_frame_v2_t* pVideoFrameNdi,
            QList<QVideoSink*>* pVideoSinks);
    void processAudio(
            NDIlib_audio_frame_v2_t* pNdiAudioFrame,
            NDIlib_audio_frame_interleaved_32f_t* pA32f,
            size_t* pnAudioBufferSize,
            QIODevice* pAudioIoDevice);
};

#endif // NDIRECEIVERWORKER_H
