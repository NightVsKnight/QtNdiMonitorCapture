#ifndef NDIWRAPPER_H
#define NDIWRAPPER_H

#include <QObject>
#include <QMap>
#include <QVideoFrame>

#include <Processing.NDI.Advanced.h>

class NdiWrapper : public QObject
{
    Q_OBJECT
public:
    static NdiWrapper &get()
    {
        static NdiWrapper instance;
        return instance;
    }

private:
    NdiWrapper(QObject *parent = nullptr);
public:
    ~NdiWrapper();

    static QString ndiFrameTypeToString(NDIlib_frame_format_type_e ndiFrameType);
    static QString ndiFourCCToString(NDIlib_FourCC_video_type_e ndiFourCC);
    static QVideoFrameFormat::PixelFormat ndiPixelFormatToQtPixelFormat(NDIlib_FourCC_video_type_e ndiFourCC);

    bool isNdiInitialized();
    bool ndiInitialize();
    void ndiDestroy();
    QMap<QString, NDIlib_source_t> ndiFindSources(bool log = true);

signals:
    //...

private:
    NDIlib_find_instance_t m_pNDI_find = nullptr;
};

#endif // NDIWRAPPER_H
