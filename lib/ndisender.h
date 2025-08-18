#pragma once

#include <QObject>
#include <QScreenCapture>
#include <QVideoFrame>

#include "Processing.NDI.Lib.h"

#include <atomic>

//
//
//

#define PIXEL_FORMAT_DX DirectXPixelFormat::B8G8R8A8UIntNormalized
#define NUM_CAPTURE_FRAME_BUFFERS 2

class NdiSender : public QObject
{
    Q_OBJECT
public:
    NdiSender(QObject *parent = nullptr);
    ~NdiSender();

    void setConnectionInfo(QString senderName, QString connectionMetadata);

    void start();
    void stop();

    void sendVideoFrame(QVideoFrame const& frame);
    void sendAudioFrame(const float* pData, int sampleCount,
                        int sampleRate, int channelCount);
    void sendMetadata(QString const& metadata);

signals:
    void onReceiverCountChanged(int receiverCount);
    void onMetadataReceived(QString metadata);

private:
    bool    m_bReconnect;
    QString m_senderName;
    QString m_connectionMetadata;
    int     m_receiverCount;

    const NDIlib_v5* m_pNdi;
    void init();

    std::atomic<NDIlib_send_instance_t> m_pNdiSend;
    uint64_t m_frameCount;
    size_t m_frameSizeBytes;
    uint8_t* m_pNdiSendBuffers[NUM_CAPTURE_FRAME_BUFFERS];
};
