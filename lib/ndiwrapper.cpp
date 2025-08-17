#include "ndiwrapper.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>


NDIlib_v5* NdiWrapper::pNdi = nullptr;
NDIlib_find_instance_t NdiWrapper::pNdiFind = nullptr;

//
//
//

QString NdiWrapper::ndiFrameTypeToString(NDIlib_frame_format_type_e ndiFrameType)
{
    QString s;
    switch(ndiFrameType)
    {
    case NDIlib_frame_format_type_e::NDIlib_frame_format_type_progressive:
        s = "NDIlib_frame_format_type_progressive";
        break;
    case NDIlib_frame_format_type_e::NDIlib_frame_format_type_interleaved:
        s = "NDIlib_frame_format_type_interleaved";
        break;
    case NDIlib_frame_format_type_e::NDIlib_frame_format_type_field_0:
        s = "NDIlib_frame_format_type_field_0";
        break;
    case NDIlib_frame_format_type_e::NDIlib_frame_format_type_field_1:
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
    QString s("NDIlib_FourCC_video_type_");
    s += (char)(ndiFourCC >> 0 & 0x000000FF);
    s += (char)(ndiFourCC >> 8 & 0x000000FF);
    s += (char)(ndiFourCC >> 16 & 0x000000FF);
    s += (char)(ndiFourCC >> 24 & 0x000000FF);
    return s + s.asprintf(" (0x%08X)", ndiFourCC);
}

QVideoFrameFormat::PixelFormat NdiWrapper::ndiPixelFormatToQtPixelFormat(NDIlib_FourCC_video_type_e ndiFourCC)
{
    using PF = QVideoFrameFormat::PixelFormat;
    switch(ndiFourCC)
    {
    case NDIlib_FourCC_video_type_UYVY: return PF::Format_UYVY;
    case NDIlib_FourCC_video_type_UYVA: return PF::Format_UYVY;
    // Result when requesting NDIlib_recv_color_format_best
    case NDIlib_FourCC_video_type_P216: return PF::Format_P016;
    case NDIlib_FourCC_video_type_PA16: return PF::Format_P016;
    case NDIlib_FourCC_video_type_YV12: return PF::Format_YV12;
    //case NDIlib_FourCC_video_type_I420: return PF::?
    case NDIlib_FourCC_video_type_NV12: return PF::Format_NV12;
    case NDIlib_FourCC_video_type_BGRA: return PF::Format_BGRA8888;
    case NDIlib_FourCC_video_type_BGRX: return PF::Format_BGRX8888;
    case NDIlib_FourCC_video_type_RGBA: return PF::Format_RGBA8888;
    case NDIlib_FourCC_video_type_RGBX: return PF::Format_RGBX8888;
    default: return PF::Format_Invalid;
    }
}

NDIlib_FourCC_video_type_e NdiWrapper::qtPixelFormatToNdiPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    using PF = QVideoFrameFormat::PixelFormat;
    switch (pixelFormat) {
    case PF::Format_ARGB8888: return NDIlib_FourCC_video_type_BGRA; // Qt: ARGB → mem order BGRA
    case PF::Format_XRGB8888: return NDIlib_FourCC_type_BGRX;
    case PF::Format_BGRA8888: return NDIlib_FourCC_type_BGRA;
    case PF::Format_BGRX8888: return NDIlib_FourCC_type_BGRX;
    case PF::Format_ABGR8888: return NDIlib_FourCC_type_RGBA; // careful: reversed order
    case PF::Format_RGBA8888: return NDIlib_FourCC_type_RGBA;
    case PF::Format_RGBX8888: return NDIlib_FourCC_type_RGBX;
    case PF::Format_NV12:     return NDIlib_FourCC_type_NV12;
    default: return NDIlib_FourCC_type_BGRX; // safe-ish fallback
    }
}

//
//
//

const NDIlib_v5* NdiWrapper::ndiGet()
{
    if (pNdi) return pNdi;

    QStringList libraryLocations;
    auto redistFolder = qEnvironmentVariable(NDILIB_REDIST_FOLDER);
    if (!redistFolder.isEmpty())
    {
        libraryLocations.push_back(redistFolder);
#if defined(__linux__) || defined(__APPLE__)
        libraryLocations.push_back("/usr/lib");
        libraryLocations.push_back("/usr/lib64");
        libraryLocations.push_back("/usr/lib/x86_64-linux-gnu");
        libraryLocations.push_back("/usr/local/lib");
        libraryLocations.push_back("/usr/local/lib64");
#endif
    }

    foreach (auto path, libraryLocations)
    {
        QFileInfo libPath(QDir(path).absoluteFilePath(NDILIB_LIBRARY_NAME));
        qDebug() << "Trying library path:" << libPath;
        if (libPath.exists() && libPath.isFile())
        {
            auto functionPointer = QLibrary::resolve(libPath.absoluteFilePath(), "NDIlib_v5_load");
            if (functionPointer)
            {
                qDebug() << "NDI5 library loaded:" << libPath;
                pNdi = ((NDIlib_v5*(*)(void))functionPointer)();
                break;
            }
            else
            {
                qDebug() << "NDIlib_v5_load not found in:" << libPath;
            }
        }
    }
    if (pNdi)
    {
        qDebug() << "NDI5 library loaded";
        if (pNdi->initialize())
        {
            qDebug() << "NDI5 initialized";

            NDIlib_find_create_t NDI_find_create_desc = { true, NULL, NULL };
            pNdiFind = pNdi->find_create_v2(&NDI_find_create_desc);
            if (pNdiFind)
            {
                qDebug() << "NDI5 global pNdi->find_create_v2(...) initialized";
            }
            else
            {
                qDebug() << "NDI5 global pNdi->find_create_v2(...) failed";
            }
        }
        else
        {
            qDebug() << "NDI5 initialize() failed. Your CPU may not be supported. Plugin disabled.";
            pNdi = nullptr;
        }
    }
    else
    {
        qDebug() << "NDI5 library not found. Plugin disabled.";
    }
    return pNdi;
}

void NdiWrapper::ndiDestroy()
{
    if (pNdi)
    {
        if (pNdiFind)
        {
            pNdi->find_destroy(pNdiFind);
            pNdiFind = nullptr;
        }

        pNdi->destroy();
        pNdi = nullptr;
    }
}

QMap<QString, NDIlib_source_t> NdiWrapper::ndiFindSources(bool log)
{
    QMap<QString, NDIlib_source_t> sources;
    if (pNdi && pNdiFind)
    {
        if (log)
        {
            qDebug() << "finding...";
        }
        uint32_t num_sources = 0;
        auto pSources = pNdi->find_get_current_sources(pNdiFind, &num_sources);
        if (pSources && num_sources)
        {
            if (log)
            {
                qDebug() << "processing" << num_sources << "NDI sources";
            }
            for (uint32_t i = 0; i < num_sources; ++i)
            {
                auto pSource = pSources[i];
                auto sourceName = QString::fromUtf8(pSource.p_ndi_name);
                //qDebug() << "source" << i << sourceName;
                sources.insert(sourceName, pSource);
            }
        }
        if (log)
        {
            qDebug() << "found" << sources.size() << "NDI sources";
        }
    }
    return sources;
}
