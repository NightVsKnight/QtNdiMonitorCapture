#pragma once

#include <QObject>
#include <QMap>
#include <QVideoFrame>

#include "Processing.NDI.Lib.h"

class NdiWrapper : public QObject
{
    Q_OBJECT
private:
    static NDIlib_v5* pNdi;
    static NDIlib_find_instance_t pNdiFind;
    NdiWrapper() : QObject(nullptr) {}
    NdiWrapper(const NdiWrapper&) = delete;
    NdiWrapper& operator= (const NdiWrapper&) = delete;

public:
    static const NDIlib_v5* ndiGet();
    static void ndiDestroy();

    static QMap<QString, NDIlib_source_t> ndiFindSources(bool log = false);

    static QString ndiFrameTypeToString(NDIlib_frame_format_type_e ndiFrameType);
    static QString ndiFourCCToString(NDIlib_FourCC_video_type_e ndiFourCC);
    static QVideoFrameFormat::PixelFormat ndiPixelFormatToQtPixelFormat(NDIlib_FourCC_video_type_e ndiFourCC);
};
