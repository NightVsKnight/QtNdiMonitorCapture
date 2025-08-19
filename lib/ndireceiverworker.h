#ifndef NDIRECEIVERWORKER_H
#define NDIRECEIVERWORKER_H

#include <QObject>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QList>
#include <QMediaDevices>
#include <QVideoFrame>
#include <QVideoSink>

#include "Processing.NDI.Lib.h"

#define MAX_AUDIO_LEVELS 16
#define AUDIO_LEVEL_MIN -60

class NdiReceiverWorker : public QObject
{
    Q_OBJECT
public:
    explicit NdiReceiverWorker(QObject* pParent = nullptr);
    ~NdiReceiverWorker();

    void setConnectionInfo(const QString& receiverName, const QString& connectionMetadata);

    QString selectedSourceName();
    void selectSource(const QString& sourceName);

    void muteAudio(bool bMute);

    void sendMetadata(const QString& metadata);

signals:
    void onSourceConnected(const QString& sourceName);
    void onMetadataReceived(const QString& metadata);
    void onVideoFrameReceived(const QVideoFrame& videoFrame);
    void onSourceDisconnected(const QString& sourceName);

public slots:
    void run();
    void stop();
    void setAudioOutputDevice(const QAudioDevice& audioDevice);

private:
    bool               m_bReconnect;
    QString            m_receiverName;
    QString            m_connectionMetadata;

    volatile bool      m_bIsRunning;

    QStringList        m_listMetadatasToSend;

    QString            m_selectedSourceName;
    volatile bool      m_bMuteAudio;
    volatile bool      m_bLowQuality;
    QString            m_cIDX;

    float              m_fAudioLevels[MAX_AUDIO_LEVELS];

    QAudioDevice       m_audioOutputDevice;
    bool               m_bAudioOutputDeviceChanged;

    void init();
    void processVideo(const NDIlib_video_frame_v2_t& pVideoFrameNdi);
    void processAudio(
            const NDIlib_v5* pNdi,
            NDIlib_audio_frame_v2_t* pNdiAudioFrame,
            NDIlib_audio_frame_interleaved_32f_t* pA32f,
            size_t* pnAudioBufferSize,
            QIODevice* pAudioIoDevice);
};

#endif // NDIRECEIVERWORKER_H
