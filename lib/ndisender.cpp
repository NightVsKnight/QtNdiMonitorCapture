#include "ndisender.h"
#include "ndiwrapper.h"

#include <QDebug>


NdiSender::NdiSender(QObject *parent)
    : QObject{parent}
    , m_pSimpleCapture{nullptr}
    , m_pNdiSend{nullptr}
    , m_frameCount{0}
    , m_frameSizeBytes{0}
    , m_pNdiSendBuffers{{}}
{
    init();
}

NdiSender::~NdiSender()
{
    stop();
}

void NdiSender::init()
{
    m_pNdi = NdiWrapper::ndiGet();

    m_bReconnect = false;
    m_senderName = "QtNdiCaptureSender";
    m_connectionMetadata = "";
    m_receiverCount = 0;

    m_pixelFormatDx = PIXEL_FORMAT_DX;
    m_pixelFormatNdi = dxPixelFormatToNdiPixelFormat(m_pixelFormatDx);
    m_pixelSizeBytes = getPixelSizeBytes(m_pixelFormatDx);
}

DirectXPixelFormat NdiSender::ndiPixelFormatToDxPixelFormat(NDIlib_FourCC_video_type_e pixelFormatNdi)
{
    switch(pixelFormatNdi)
    {
    case NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_BGRA:
        return DirectXPixelFormat::B8G8R8A8UIntNormalized;
    // TODO: Add more formats as necessary
    default:
        throw std::runtime_error(QString("Unhandled pixel format %1").arg((int)pixelFormatNdi).toStdString());
    }
}

NDIlib_FourCC_video_type_e NdiSender::dxPixelFormatToNdiPixelFormat(DirectXPixelFormat pixelFormatDx)
{
    switch(pixelFormatDx)
    {
    case DirectXPixelFormat::B8G8R8A8UIntNormalized:
        return NDIlib_FourCC_video_type_e::NDIlib_FourCC_video_type_BGRA;
    // TODO: Add more formats as necessary
    default:
        throw std::runtime_error(QString("Unhandled pixel format %1").arg((int)pixelFormatDx).toStdString());
    }
}

int NdiSender::getPixelSizeBytes(DirectXPixelFormat pixelFormatDx)
{
    switch(pixelFormatDx)
    {
    case DirectXPixelFormat::B8G8R8A8UIntNormalized:
        return 4;
    // TODO: Add more formats as necessary
    default:
        throw std::runtime_error(QString("Unhandled pixel format %1").arg((int)pixelFormatDx).toStdString());
    }
}

bool NdiSender::isCapturing()
{
    return m_pSimpleCapture != nullptr;
}

void NdiSender::setConnectionInfo(QString senderName, QString connectionMetadata)
{
    m_senderName = senderName;
    m_connectionMetadata = connectionMetadata;
    m_bReconnect = true;
}

void NdiSender::stop()
{
    qDebug() << "+stop()";
    auto pSimpleCapture = m_pSimpleCapture.exchange(nullptr);
    if (pSimpleCapture)
    {
        pSimpleCapture->Close();
        pSimpleCapture = nullptr;
    }
    auto pNdiSend = m_pNdiSend.exchange(nullptr);
    if (pNdiSend)
    {
        m_pNdi->send_send_video_async_v2(pNdiSend, NULL);
        m_pNdi->send_destroy(pNdiSend);
        pNdiSend = nullptr;
    }
    for (int i = 0; i < NUM_CAPTURE_FRAME_BUFFERS; ++i)
    {
        if (m_pNdiSendBuffers[i])
        {
            delete[] m_pNdiSendBuffers[i];
            m_pNdiSendBuffers[i] = nullptr;
        }
    }
    m_frameCount = 0;
    m_frameSizeBytes = 0;
    m_receiverCount = 0;
    qDebug() << "-stop()";
}

void NdiSender::start(HMONITOR hmonitor)
{
    qDebug() << "+start(...)";

    stop();

    if (hmonitor == 0) return;

    auto item = CreateCaptureItemForMonitor(hmonitor);

    m_pSimpleCapture = new SimpleCapture();
    (*m_pSimpleCapture).StartCapture(item, m_pixelFormatDx, m_pixelSizeBytes, NUM_CAPTURE_FRAME_BUFFERS,
         this,
         static_cast<SimpleCapture::CALLBACK_ON_FRAME>(NdiSender::onFrameReceived),
         static_cast<SimpleCapture::CALLBACK_ON_FRAME_BUFFER>(NdiSender::onFrameReceivedBuffer));

    NDIlib_send_create_t NDI_send_create_desc;
    auto utf8 = m_senderName.toUtf8();
    NDI_send_create_desc.p_ndi_name = utf8.constData();
    m_pNdiSend = m_pNdi->send_create(&NDI_send_create_desc);
    Q_ASSERT(m_pNdiSend);

    NDIlib_metadata_frame_t connection_metadata;
    utf8 = m_connectionMetadata.toUtf8();
    connection_metadata.p_data = (char*)utf8.constData();
    m_pNdi->send_add_connection_metadata(m_pNdiSend, &connection_metadata);

    // TODO: Capture and send **Audio** in dedicated thread...

    qDebug() << "-start(...)";
}

bool NdiSender::onFrameReceived(
        SimpleCapture*,
        Direct3D11CaptureFrame const& frame)
{
    auto pNdiSend = m_pNdiSend.load();
    if (!pNdiSend) return false;

    auto receiverCount = m_pNdi->send_get_no_connections(pNdiSend, 0);
    //qDebug() << "receiverCount" << receiverCount;
    if (receiverCount != m_receiverCount)
    {
        emit onReceiverCountChanged(receiverCount);
        m_receiverCount = receiverCount;
    }
    if (receiverCount == 0)
    {
        return false;
    }

    if (frame)
    {
        //qDebug() << ".";

        //
        // METADATA RECV
        //
        NDIlib_metadata_frame_t metadata_frame;
        switch(m_pNdi->send_capture(pNdiSend, &metadata_frame, 0))
        {
        case NDIlib_frame_type_e::NDIlib_frame_type_metadata:
        {
            auto metadata = QString::fromUtf8(metadata_frame.p_data);
            m_pNdi->send_free_metadata(pNdiSend, &metadata_frame);
            qDebug() << "pNdi->send_capture NDIlib_frame_type_metadata" << metadata;
            emit onMetadataReceived(metadata);
            break;
        }
        case NDIlib_frame_type_e::NDIlib_frame_type_status_change:
            qDebug() << "pNdi->send_capture NDIlib_frame_type_status_change";
            break;
        default:
            // ignore
            break;
        }
    }
    else
    {
        // End of capture
        m_pNdi->send_send_video_async_v2(pNdiSend, NULL);
    }

    return true;
}

void NdiSender::onFrameReceivedBuffer(
        SimpleCapture*,
        int frameWidth,
        int frameHeight,
        int frameStrideBytes,
        void* pFrameBuffer)
{
    auto pNdiSend = m_pNdiSend.load();
    if (!pNdiSend || !pFrameBuffer) return;

    auto thisFrameSizeBytes = frameStrideBytes * frameHeight;
    if (m_frameSizeBytes < thisFrameSizeBytes)
    {
        qDebug() << "growing m_pNdiSendBuffers from" << m_frameSizeBytes << "to" << thisFrameSizeBytes << "bytes";
        m_frameSizeBytes = thisFrameSizeBytes;
        for (int i = 0; i < NUM_CAPTURE_FRAME_BUFFERS; ++i)
        {
            if (m_pNdiSendBuffers[i])
            {
                delete[] m_pNdiSendBuffers[i];
            }
            m_pNdiSendBuffers[i] = new uint8_t[thisFrameSizeBytes];
        }
    }

    uint8_t* pOutBuffer = m_pNdiSendBuffers[m_frameCount++ % NUM_CAPTURE_FRAME_BUFFERS];
    memcpy(pOutBuffer, pFrameBuffer, thisFrameSizeBytes);

    NDIlib_video_frame_v2_t NDI_video_frame;
    NDI_video_frame.xres = frameWidth;
    NDI_video_frame.yres = frameHeight;
    NDI_video_frame.FourCC = m_pixelFormatNdi;
    NDI_video_frame.line_stride_in_bytes = frameStrideBytes;
    NDI_video_frame.p_data = pOutBuffer;

    m_pNdi->send_send_video_async_v2(pNdiSend, &NDI_video_frame);
}

void NdiSender::sendMetadata(QString metadata)
{
    auto pNdiSend = m_pNdiSend.load();
    if (!pNdiSend) return;
    NDIlib_metadata_frame_t metadata_frame;
    auto utf8 = metadata.toUtf8();
    metadata_frame.p_data = (char*)utf8.constData();
    qDebug() << "pNdi->send_send_metadata" << metadata;
    m_pNdi->send_send_metadata(pNdiSend, &metadata_frame);
}
