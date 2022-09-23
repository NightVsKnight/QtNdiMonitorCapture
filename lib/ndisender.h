#pragma once

#include <QObject>

#include <Processing.NDI.Advanced.h>

#include "SimpleCapture.h"

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::UI::Composition;

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

    bool isCapturing();

    void setConnectionInfo(QString senderName, QString connectionMetadata);

    void start(HMONITOR hmonitor);
    void stop();

    void sendMetadata(QString metadata);

signals:
    void onReceiverCountChanged(int receiverCount);
    void onMetadataReceived(QString metadata);

private:
    bool    m_bReconnect;
    QString m_senderName;
    QString m_connectionMetadata;
    int     m_receiverCount;

    void init();

    static DirectXPixelFormat ndiPixelFormatToDxPixelFormat(NDIlib_FourCC_video_type_e pixelFormatNdi);
    static NDIlib_FourCC_video_type_e dxPixelFormatToNdiPixelFormat(DirectXPixelFormat pixelFormatDx);
    static int getPixelSizeBytes(DirectXPixelFormat pixelFormatDx);

    DirectXPixelFormat         m_pixelFormatDx;
    NDIlib_FourCC_video_type_e m_pixelFormatNdi;
    int                        m_pixelSizeBytes;

    Windows::System::DispatcherQueueController m_dispatcherQueueController;
    Windows::System::DispatcherQueue m_dispatcherQueue;
    atomic<SimpleCapture*> m_pSimpleCapture;
    atomic<NDIlib_send_instance_t> m_pNdiSend;
    uint64_t m_frameCount;
    size_t m_frameSizeBytes;
    uint8_t* m_pNdiSendBuffers[NUM_CAPTURE_FRAME_BUFFERS];

    static bool onFrameReceived(void* pObj, SimpleCapture* pSender, Direct3D11CaptureFrame frame)
    {
        return ((NdiSender*)pObj)->onFrameReceived(pSender, frame);
    }
    bool onFrameReceived(SimpleCapture* sender, Direct3D11CaptureFrame frame);

    static void onFrameReceivedBuffer(void* pObj, SimpleCapture* pSender, uint width, uint height, uint strideBytes, void* pFrameBuffer)
    {
        ((NdiSender*)pObj)->onFrameReceivedBuffer(pSender, width, height, strideBytes, pFrameBuffer);
    }
    void onFrameReceivedBuffer(SimpleCapture* pSender, uint frameWidth, uint frameHeight, uint frameStrideBytes, void* pFrameBuffer);
};
