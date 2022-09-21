#include "ndiwrapper.h"

#include <QDebug>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.Advanced.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.Advanced.x86.lib")
#endif // _WIN64
#endif


NdiWrapper::NdiWrapper(QObject *parent) : QObject(parent)
{
}

NdiWrapper::~NdiWrapper()
{
    ndiDestroy();
}

//
//
//

QString NdiWrapper::ndiFrameTypeToString(NDIlib_frame_format_type_e ndiFrameType)
{
    QString s;
    switch(ndiFrameType)
    {
    case NDIlib_frame_format_type_progressive:
        s = "NDIlib_frame_format_type_progressive";
        break;
    case NDIlib_frame_format_type_interleaved:
        s = "NDIlib_frame_format_type_interleaved";
        break;
    case NDIlib_frame_format_type_field_0:
        s = "NDIlib_frame_format_type_field_0";
        break;
    case NDIlib_frame_format_type_field_1:
        s = "NDIlib_frame_format_type_field_1";
        break;
    default:
        s = "UNKNOWN";
        break;
    }
    return s + s.asprintf(" (0x%X)", ndiFrameType);
}

/**
 * Inverse of the NDI_LIB_FOURCC macro defined in `Processing.NDI.structs.h`
 */
QString NdiWrapper::ndiFourCCToString(NDIlib_FourCC_video_type_e ndiFourCC)
{
    QString s;
    s += (char)(ndiFourCC >> 0 & 0x000000FF);
    s += (char)(ndiFourCC >> 8 & 0x000000FF);
    s += (char)(ndiFourCC >> 16 & 0x000000FF);
    s += (char)(ndiFourCC >> 24 & 0x000000FF);
    return s + s.asprintf(" (0x%08X)", ndiFourCC);
}

QVideoFrameFormat::PixelFormat NdiWrapper::ndiPixelFormatToPixelFormat(enum NDIlib_FourCC_video_type_e ndiFourCC)
{
    switch(ndiFourCC)
    {
    case NDIlib_FourCC_video_type_UYVY:
        return QVideoFrameFormat::PixelFormat::Format_UYVY;
    case NDIlib_FourCC_video_type_UYVA:
        return QVideoFrameFormat::PixelFormat::Format_UYVY;
        break;
    // Result when requesting NDIlib_recv_color_format_best
    case NDIlib_FourCC_video_type_P216:
        return QVideoFrameFormat::PixelFormat::Format_P016;
    //case NDIlib_FourCC_video_type_PA16:
    //    return QVideoFrameFormat::PixelFormat::?;
    case NDIlib_FourCC_video_type_YV12:
        return QVideoFrameFormat::PixelFormat::Format_YV12;
    //case NDIlib_FourCC_video_type_I420:
    //    return QVideoFrameFormat::PixelFormat::?
    case NDIlib_FourCC_video_type_NV12:
        return QVideoFrameFormat::PixelFormat::Format_NV12;
    case NDIlib_FourCC_video_type_BGRA:
        return QVideoFrameFormat::PixelFormat::Format_BGRA8888;
    case NDIlib_FourCC_video_type_BGRX:
        return QVideoFrameFormat::PixelFormat::Format_BGRX8888;
    case NDIlib_FourCC_video_type_RGBA:
        return QVideoFrameFormat::PixelFormat::Format_RGBA8888;
    case NDIlib_FourCC_video_type_RGBX:
        return QVideoFrameFormat::PixelFormat::Format_RGBX8888;
    default:
        return QVideoFrameFormat::PixelFormat::Format_Invalid;
    }
}

//
//
//

bool NdiWrapper::isNdiInitialized()
{
    return m_pNDI_find != nullptr;
}

bool NdiWrapper::ndiInitialize()
{
    if (isNdiInitialized()) return true;

    auto initialized = NDIlib_initialize();
    Q_ASSERT(initialized);

    const NDIlib_find_create_t NDI_find_create_desc = { true, NULL, NULL };
    m_pNDI_find = NDIlib_find_create_v2(&NDI_find_create_desc);
    Q_ASSERT(m_pNDI_find != nullptr);

    return isNdiInitialized();
}

void NdiWrapper::ndiDestroy()
{
    if (!isNdiInitialized()) return;

    NDIlib_find_destroy(m_pNDI_find);
    m_pNDI_find = nullptr;

    NDIlib_destroy();
}

QMap<QString, NDIlib_source_t> NdiWrapper::ndiFindSources(bool log)
{
    QMap<QString, NDIlib_source_t> _map;
    if (!isNdiInitialized()) goto exit;
    {
        if (log)
        {
            qDebug() << "finding...";
        }
        uint32_t num_sources = 0;
        const NDIlib_source_t* p_sources = NDIlib_find_get_current_sources(m_pNDI_find, &num_sources);
        if (num_sources) {
            if (log)
            {
                qDebug() << "processing" << num_sources << "NDI sources";
            }
            for (uint32_t i = 0; i < num_sources; ++i) {
                NDIlib_source_t p_source = p_sources[i];
                QString cNdiName = QString::fromUtf8(p_source.p_ndi_name);
                //qDebug() << "processing source" << i << cNdiName;
                _map.insert(cNdiName, p_source);
            }
        }
        if (log)
        {
            qDebug() << "found" << _map.size() << "NDI sources";
        }
    }
exit:
    return _map;
}
