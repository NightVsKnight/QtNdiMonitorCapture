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
    explicit NdiReceiverWorker(QObject *parent = nullptr);
    ~NdiReceiverWorker();

    void setConnectionInfo(QString receiverName, QString connectionMetadata);

    void addVideoSink(QVideoSink *videoSink);
    void removeVideoSink(QVideoSink *videoSink);
    QString getNdiSourceName();
    void setNdiSourceName(QString cNdiSourceName);

    void muteAudio(bool bMute);

signals:
    void ndiSourceConnected();
    void ndiSourceDisconnected();
    void audioLevelLeftChanged(int);
    void audioLevelRightChanged(int);

public slots:
    void stop();
    void process();

private:
    bool          m_bReconnect;
    QString       m_receiverName;
    QString       m_connectionMetadata;

    QList<QVideoSink*> m_videoSinks;

    bool m_bIsProcessing = false;

    QString       m_cNdiSourceName;
    int           m_nAudioLevelLeft;
    int           m_nAudioLevelRight;
    volatile bool m_bMuteAudio;
    volatile bool m_bLowQuality;
    QString       m_cIDX;

    float m_fAudioLevels[MAX_AUDIO_LEVELS];

    void init();
    void setAudioLevelLeft(int level);
    void setAudioLevelRight(int level);
    void processVideo(NDIlib_video_frame_v2_t *pNdiVideoFrame, QList<QVideoSink*> *videoSinks);
    void processAudio(NDIlib_audio_frame_v2_t *pNdiAudioFrame, NDIlib_audio_frame_interleaved_32f_t *pA32f, size_t *pnAudioBufferSize, QIODevice *pAudioIoDevice);
    void processMetaData(NDIlib_metadata_frame_t *pNdiMetadataFrame);
};

#endif // NDIRECEIVERWORKER_H
