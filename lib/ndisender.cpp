#include "ndisender.h"
#include "ndiwrapper.h"

#include <QDebug>


NdiSender::NdiSender(QObject *parent)
    : QObject{parent}
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
    m_senderName = "QtNdiMonitorCaptureSender";
    m_connectionMetadata = "";
    m_receiverCount = 0;
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

void NdiSender::start()
{
    qDebug() << "+start()";

    stop();

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

    qDebug() << "-start()";
}

/*
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
*/

void NdiSender::sendVideoFrame(QVideoFrame const& frame)
{
    auto pNdiSend = m_pNdiSend.load();
    if (!pNdiSend) return;

    auto receiverCount = m_pNdi->send_get_no_connections(pNdiSend, 0);
    //qDebug() << "receiverCount" << receiverCount;
    if (receiverCount != m_receiverCount)
    {
        emit onReceiverCountChanged(receiverCount);
        m_receiverCount = receiverCount;
    }
    if (receiverCount == 0) return;

    if (!frame.isValid()) return;

    QVideoFrame f(frame); // cheap refcount
    if (!f.map(QVideoFrame::ReadOnly)) return;

    auto frameWidth = frame.width();
    auto frameHeight = frame.height();
    auto frameStrideBytes = frame.bytesPerLine(0); // returns 0 if [above] `map` is not called
    auto framePixelFormat = frame.pixelFormat();
    //qDebug() << "framePixelFormat=" << framePixelFormat;
    auto ndiPixelFormat = NdiWrapper::qtPixelFormatToNdiPixelFormat(framePixelFormat);
    //qDebug() << "ndiPixelFormat=" << NdiWrapper::ndiFourCCToString(ndiPixelFormat);

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
    memcpy(pOutBuffer, frame.bits(0), thisFrameSizeBytes);

    NDIlib_video_frame_v2_t ndi_frame;
    ndi_frame.xres = frameWidth;
    ndi_frame.yres = frameHeight;
    ndi_frame.FourCC = ndiPixelFormat;
    ndi_frame.line_stride_in_bytes = frameStrideBytes;
    ndi_frame.p_data = pOutBuffer;
    //ndi_frame.frame_rate_N = 60; // Or detect via screen refresh
    //ndi_frame.frame_rate_D = 1;
    //ndi_frame.timecode = NDIlib_send_timecode_synthesize;

    m_pNdi->send_send_video_async_v2(pNdiSend, &ndi_frame);

    f.unmap();
}

void NdiSender::sendMetadata(QString const& metadata)
{
    auto pNdiSend = m_pNdiSend.load();
    if (!pNdiSend) return;
    NDIlib_metadata_frame_t metadata_frame;
    auto utf8 = metadata.toUtf8();
    metadata_frame.p_data = (char*)utf8.constData();
    qDebug() << "pNdi->send_send_metadata" << metadata;
    m_pNdi->send_send_metadata(pNdiSend, &metadata_frame);
}
