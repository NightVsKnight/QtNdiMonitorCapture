#include "ndireceiverworker.h"

#include <QDateTime>
#include <QPainter>
#include <QThread>

#include "ndiwrapper.h"

NdiReceiverWorker::NdiReceiverWorker(QObject* parent)
    : QObject(parent)
{
    init();
}

NdiReceiverWorker::~NdiReceiverWorker()
{
    qDebug() << "+ ~NdiReceiverWorker()";

    stop();

    // TODO: Is there a better way for this worker [non-QThread] QObject to exit?
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    qDebug() << "- ~NdiReceiverWorker()";
}

void NdiReceiverWorker::init()
{
    qDebug() << "+init()";
    m_bReconnect = false;
    m_receiverName = "QtNdiMonitorReceiver";
    m_connectionMetadata = "";
    m_bIsProcessing = false;
    m_cNdiSourceName.clear();
    m_nAudioLevelLeft = AUDIO_LEVEL_MIN;
    m_nAudioLevelRight = AUDIO_LEVEL_MIN;
    m_bMuteAudio = false;
    m_cIDX.clear();
    std::fill(std::begin(m_fAudioLevels), std::end(m_fAudioLevels), 0.0);
    qDebug() << "-init()";
}

void NdiReceiverWorker::setConnectionInfo(QString receiverName, QString connectionMetadata)
{
    m_receiverName = receiverName;
    m_connectionMetadata = connectionMetadata;
    m_bReconnect = true;
}

void NdiReceiverWorker::addVideoSink(QVideoSink *videoSink)
{
    if (videoSink && !m_videoSinks.contains(videoSink))
    {
        m_videoSinks.append(videoSink);
    }
}

void NdiReceiverWorker::removeVideoSink(QVideoSink *videoSink)
{
    if (videoSink)
    {
        m_videoSinks.removeAll(videoSink);
    }
}

QString NdiReceiverWorker::getNdiSourceName()
{
    return m_cNdiSourceName;
}

void NdiReceiverWorker::setNdiSourceName(QString cNdiSourceName)
{
    m_cNdiSourceName = cNdiSourceName;
}

void NdiReceiverWorker::muteAudio(bool bMute)
{
    m_bMuteAudio = bMute;
}

void NdiReceiverWorker::setAudioLevelLeft(int level)
{
    m_nAudioLevelLeft = level;
    Q_EMIT audioLevelLeftChanged(m_nAudioLevelLeft);
}

void NdiReceiverWorker::setAudioLevelRight(int level)
{
    m_nAudioLevelRight = level;
    Q_EMIT audioLevelRightChanged(m_nAudioLevelRight);
}

void NdiReceiverWorker::stop()
{
    qDebug() << "+stop()";
    m_bIsProcessing = false;
    qDebug() << "-stop()";
}

void NdiReceiverWorker::process()
{
    qDebug() << "+process()";

    if (m_bIsProcessing) return;
    m_bIsProcessing = true;

    QAudioSink *pAudioOutputSink = nullptr;
    QIODevice *pAudioIoDevice = nullptr;

    QAudioFormat audioFormat;
    audioFormat.setSampleRate(44100);
    audioFormat.setChannelCount(2);
    audioFormat.setSampleFormat(QAudioFormat::Float);
    //format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    qDebug() << "audioFormat" << audioFormat;

    auto audioOutputDevice = QMediaDevices::defaultAudioOutput();
    if (audioOutputDevice.isFormatSupported(audioFormat))
    {
        pAudioOutputSink = new QAudioSink(audioOutputDevice, audioFormat, this);
        pAudioOutputSink->setVolume(1.0);
        pAudioIoDevice = pAudioOutputSink->start();
    }
    else
    {
        qWarning() << "process: Requested audio format is not supported by the default audio device.";
    }

    NDIlib_video_frame_v2_t video_frame;
    NDIlib_audio_frame_v2_t audio_frame;
    NDIlib_metadata_frame_t metadata_frame;
    NDIlib_audio_frame_interleaved_32f_t a32f;
    size_t nAudioBufferSize = 0;

    QString                     cNdiSourceName;
    int64_t                     nVTimestamp = 0;
    int64_t                     nATimestamp = 0;
    NDIlib_recv_instance_t      pNdiRecv = nullptr;
    NDIlib_framesync_instance_t pNdiFrameSync = nullptr;

    bool isConnected = false;

    while (m_bIsProcessing)
    {
        //
        // TODO: CLEAN UP THIS MESS: BEGIN!
        //

        /*
        // Is anything selected
        if (m_cNdiSourceName.isEmpty())
        {
            qDebug() << "No NDI source selected; continue";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        */

        if (m_bReconnect || (cNdiSourceName != m_cNdiSourceName))
        {
            m_bReconnect = false;

            // Change source

            //cfg->setRecStatus(0);

            // TODO: emit source changing...
            /*
            emit newVideoFrame();
            setAudioLevelLeft(AUDIO_LEVEL_MIN);
            setAudioLevelRight(AUDIO_LEVEL_MIN);
            */

            auto ndiSources = NdiWrapper::get().ndiFindSources(false);
            auto ndiList = ndiSources.values();
            if (!ndiList.size())
            {
                if (true)
                {
                    qDebug() << "No NDI sources; continue";
                }
                // TODO: Tune this duration?
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            int nSourceChannel = -1;
            for (int i = 0; i < ndiSources.size(); i++)
            {
                auto _cNdiSourceName = QString::fromUtf8(ndiList.at(i).p_ndi_name);
                if (QString::compare(_cNdiSourceName, m_cNdiSourceName, Qt::CaseInsensitive) == 0)
                {
                    nSourceChannel = i;
                    cNdiSourceName = _cNdiSourceName;
                    break;
                }
            }
            if (nSourceChannel == -1)
            {
                qDebug() << "Selected NDI source not available; continue;";
                if (isConnected)
                {
                    isConnected = false;
                    emit ndiSourceDisconnected();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (true)
            {
                qDebug() << "NDI source activated: " << cNdiSourceName;
            }

            if (true)
            {
                qDebug() << "NDI init starts...";
            }

            if (pNdiFrameSync != nullptr)
            {
                NDIlib_framesync_destroy(pNdiFrameSync);
                pNdiFrameSync = nullptr;
            }

            if (pNdiRecv != nullptr)
            {
                if (pAudioOutputSink && m_bIsProcessing)
                {
                    if (true)
                    {
                        qDebug() << "NDI audio stop.";
                    }
                    pAudioOutputSink->stop();
                    pAudioIoDevice = nullptr;
                }

                NDIlib_recv_destroy(pNdiRecv);
                pNdiRecv = nullptr;
            }

            NDIlib_recv_create_v3_t recv_desc;
            auto utf8 = m_receiverName.toUtf8();
            recv_desc.p_ndi_recv_name = utf8.constData();
            recv_desc.source_to_connect_to = ndiList.at(nSourceChannel);
            //recv_desc.color_format = NDIlib_recv_color_format_UYVY_BGRA;
            recv_desc.color_format = NDIlib_recv_color_format_best;
            recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
            recv_desc.allow_video_fields = true;
            if (true)
            {
                qDebug().nospace() << "NDI_recv_create_desc.source_to_connect_to=" << QString(recv_desc.source_to_connect_to.p_ndi_name);
                qDebug().nospace() << "NDI_recv_create_desc.color_format=" << recv_desc.color_format;
                qDebug().nospace() << "NDI_recv_create_desc.bandwidth=" << recv_desc.bandwidth;
            }

            qDebug() << "NDIlib_recv_create_v3";
            pNdiRecv = NDIlib_recv_create_v3(&recv_desc);
            if (pNdiRecv == nullptr)
            {
                qDebug() << "NDIlib_recv_create_v3 failed; should probably throw an exception here...";
                break;
            }

            nVTimestamp = 0;
            nATimestamp = 0;

            qDebug() << "NDIlib_framesync_create";
            pNdiFrameSync = NDIlib_framesync_create(pNdiRecv);
            if (pNdiFrameSync == nullptr)
            {
                qDebug() << "NDIlib_framesync_create failed; should probably throw an exception here...";
                break;
            }

            NDIlib_recv_clear_connection_metadata(pNdiRecv);

            NDIlib_metadata_frame_t enable_hw_accel;
            enable_hw_accel.p_data = (char*)"<ndi_video_codec type=\"hardware\"/>";
            qDebug() << "NDIlib_recv_send_metadata enable_hw_accel" << enable_hw_accel.p_data;
            NDIlib_recv_send_metadata(pNdiRecv, &enable_hw_accel);

            if (!m_connectionMetadata.isEmpty())
            {
                NDIlib_metadata_frame_t connection_metadata;
                auto utf8 = m_connectionMetadata.toUtf8();
                connection_metadata.p_data = (char*)utf8.constData();
                qDebug() << "NDIlib_recv_add_connection_metadata connection_metadata" << connection_metadata.p_data;
                NDIlib_recv_add_connection_metadata(pNdiRecv, &connection_metadata);
            }

            //cfg->setRecStatus(1);

            if (pAudioOutputSink && m_bIsProcessing)
            {
                if (pAudioIoDevice == nullptr)
                {
                    pAudioIoDevice = pAudioOutputSink->start();
                    if (pAudioIoDevice && true)
                    {
                        qDebug() << "NDI receiver audio started.";
                    }
                }
            }
        }

        if (cNdiSourceName.isEmpty())
        {
            qDebug() << "No NDI source selected; continue";
            if (isConnected)
            {
                isConnected = false;
                emit ndiSourceDisconnected();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        else
        {
            //if ()
        }

        if (NDIlib_recv_get_no_connections(pNdiRecv) == 0)
        {
            if (isConnected)
            {
                isConnected = false;
                emit ndiSourceDisconnected();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        if (!isConnected)
        {
            isConnected = true;
            emit ndiSourceConnected();
        }

        //cNdiSourceName = m_cNdiSourceName;

        //
        // TODO: CLEAN UP THIS MESS: END!
        //

        //
        // VIDEO
        //
        video_frame = {};
        //qDebug() << "NDIlib_framesync_capture_video";
        NDIlib_framesync_capture_video(pNdiFrameSync, &video_frame, NDIlib_frame_format_type_progressive);
        //qDebug() << "video_frame.p_data=" << video_frame.p_data;
        if (video_frame.p_data && (video_frame.timestamp > nVTimestamp))
        {
            //qDebug() << "video_frame";
            nVTimestamp = video_frame.timestamp;
            processVideo(&video_frame, &m_videoSinks);
        }
        NDIlib_framesync_free_video(pNdiFrameSync, &video_frame);

        //
        // AUDIO
        //
        audio_frame = {};
        //qDebug() << "NDIlib_framesync_capture_audio";
        NDIlib_framesync_capture_audio(pNdiFrameSync, &audio_frame, audioFormat.sampleRate(), audioFormat.channelCount(), 1024);
        //qDebug() << "audio_frame.p_data=" << audio_frame.p_data;
        if (audio_frame.p_data && (audio_frame.timestamp > nATimestamp))
        {
            //qDebug() << "audio_frame";
            nATimestamp = audio_frame.timestamp;
            processAudio(&audio_frame, &a32f, &nAudioBufferSize, pAudioIoDevice);
        }
        NDIlib_framesync_free_audio(pNdiFrameSync, &audio_frame);

        //
        // METADATA
        //
        auto frameType = NDIlib_recv_capture_v2(pNdiRecv, nullptr, nullptr, &metadata_frame, 0);
        switch(frameType)
        {
        case NDIlib_frame_type_e::NDIlib_frame_type_metadata:
            qDebug() << "NDIlib_frame_type_metadata";
            processMetaData(&metadata_frame);
            break;
        case NDIlib_frame_type_e::NDIlib_frame_type_status_change:
            qDebug() << "NDIlib_frame_type_status_change";
            break;
        default:
            // ignore
            break;
        }

        // TODO: More accurate sleep that subtracts the duration of this loop
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (a32f.p_data)
    {
        delete[] a32f.p_data;
        a32f.p_data = nullptr;
    }

    if (pNdiFrameSync)
    {
        NDIlib_framesync_destroy(pNdiFrameSync);
        pNdiFrameSync = nullptr;
    }

    if (pNdiRecv)
    {
        NDIlib_recv_destroy(pNdiRecv);
        pNdiRecv = nullptr;
    }

    if (pAudioOutputSink)
    {
        pAudioOutputSink->stop();
        delete pAudioOutputSink;
        pAudioOutputSink = nullptr;
        pAudioIoDevice = nullptr;
    }

    qDebug() << "-process()";
}

void NdiReceiverWorker::processMetaData(NDIlib_metadata_frame_t *pNdiMetadataFrame)
{
    QString metadata = QString::fromUtf8(pNdiMetadataFrame->p_data, pNdiMetadataFrame->length);
    qDebug() << "metadata" << metadata;
}

//#define MY_VERBOSE_LOGGING

QString withCommas(int value)
{
    return QLocale(QLocale::English).toString(value);
}

void NdiReceiverWorker::processVideo(
        NDIlib_video_frame_v2_t *pNdiVideoFrame,
        QList<QVideoSink*> *videoSinks)
{
    auto ndiWidth = pNdiVideoFrame->xres;
    auto ndiHeight = pNdiVideoFrame->yres;
    auto ndiLineStrideInBytes = pNdiVideoFrame->line_stride_in_bytes;
    auto ndiPixelFormat = pNdiVideoFrame->FourCC;
#if defined(MY_VERBOSE_LOGGING)
    qDebug();
    qDebug() << "+processVideo(...)";
    auto ndiArea = ndiWidth * ndiHeight;
    auto ndiFrameType = pNdiVideoFrame->frame_format_type;
    qDebug().noquote() << "pNdiVideoFrame Size (X x Y = Pixels):" << withCommas(ndiWidth) << "x" << withCommas(ndiHeight) << "=" << withCommas(ndiArea);
    qDebug() << "pNdiVideoFrame->line_stride_in_bytes" << withCommas(ndiLineStrideInBytes);
    qDebug() << "pNdiVideoFrame->frame_format_type" << ndiFrameTypeToString(ndiFrameType);
    qDebug() << "pNdiVideoFrame->FourCC:" << ndiFourCCToString(ndiPixelFormat);
#endif
    auto pixelFormat = NdiWrapper::ndiPixelFormatToPixelFormat(ndiPixelFormat);
#if defined(MY_VERBOSE_LOGGING) && false
    qDebug() << "pixelFormat" << QVideoFrameFormat::pixelFormatToString(pixelFormat);
#endif
    if (pixelFormat == QVideoFrameFormat::PixelFormat::Format_Invalid)
    {
        qDebug().nospace() << "Unsupported pNdiVideoFrame->FourCC " << NdiWrapper::ndiFourCCToString(ndiPixelFormat) << "; return;";
        return;
    }

    QSize videoFrameSize(ndiWidth, ndiHeight);
    QVideoFrameFormat videoFrameFormat(videoFrameSize, pixelFormat);
    //videoFrameFormat.setYCbCrColorSpace(QVideoFrameFormat::YCbCrColorSpace::?); // Does this make any difference?
    QVideoFrame videoFrame(videoFrameFormat);

    if (!videoFrame.map(QVideoFrame::WriteOnly))
    {
        qWarning() << "videoFrame.map(QVideoFrame::WriteOnly) failed; return;";
        return;
    }

    auto pDstY = videoFrame.bits(0);
    auto pSrcY = pNdiVideoFrame->p_data;
    auto pDstUV = videoFrame.bits(1);
    auto pSrcUV = pSrcY + (ndiLineStrideInBytes * ndiHeight);
    for (int line = 0; line < ndiHeight; ++line)
    {
        memcpy(pDstY, pSrcY, ndiLineStrideInBytes);
        pDstY += ndiLineStrideInBytes;
        pSrcY += ndiLineStrideInBytes;

        if (pDstUV)
        {
            // For now QVideoFrameFormat/QVideoFrame does not support P216. :(
            // I have started the conversation to have it added, but that may take awhile. :(
            // Until then, copying only every other UV line is a cheap way to downsample P216's 4:2:2 to P016's 4:2:0 chroma sampling.
            // There are still a few visible artifacts on the screen, but it is passable.
            if (line % 2)
            {
                memcpy(pDstUV, pSrcUV, ndiLineStrideInBytes);
                pDstUV += ndiLineStrideInBytes;
            }
            pSrcUV += ndiLineStrideInBytes;
        }
    }

#if false
    // TODO: Find way to overlay FPS counter
    QImage::Format image_format = QVideoFrameFormat::imageFormatFromPixelFormat(outff.pixelFormat());
    QImage image(outff.bits(0), video_frame->xres, video_frame->yres, image_format);
    image.fill(Qt::green);

    QPainter painter(&image);
    painter.drawText(image.rect(), Qt::AlignCenter, QDateTime::currentDateTime().toString());
    painter.end();
#endif

    videoFrame.unmap();

    for (auto videoSink : *videoSinks)
    {
        videoSink->setVideoFrame(videoFrame);
    }

#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "-processVideo(...)";
#endif
}

void NdiReceiverWorker::processAudio(
        NDIlib_audio_frame_v2_t *pNdiAudioFrame,
        NDIlib_audio_frame_interleaved_32f_t *pA32f,
        size_t *pnAudioBufferSize,
        QIODevice *pAudioIoDevice)
{
#if defined(MY_VERBOSE_LOGGING)
    qDebug();
    qDebug() << "+processAudio(...)";
#endif
    if (m_bMuteAudio)
    {
        qDebug() << "m_bMuteAudio == true; return;";
        return;
    }
    if (pAudioIoDevice == nullptr)
    {
        qDebug() << "pAudioIoDevice == nullptr; return;";
        return;
    }

    size_t nThisAudioBufferSize = pNdiAudioFrame->no_samples * pNdiAudioFrame->no_channels * sizeof(float);
#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "nThisAudioBufferSize" << nThisAudioBufferSize;
#endif
    if (*pnAudioBufferSize < nThisAudioBufferSize)
    {
        qDebug() << "growing pA32f->p_data from" << *pnAudioBufferSize << "to" << nThisAudioBufferSize << "bytes";
        *pnAudioBufferSize = nThisAudioBufferSize;
        if (pA32f->p_data)
        {
            delete[] pA32f->p_data;
        }
        pA32f->p_data = new float[nThisAudioBufferSize];
    }

    NDIlib_util_audio_to_interleaved_32f_v2(pNdiAudioFrame, pA32f);

    size_t nWritten = 0;
    do
    {
        nWritten += pAudioIoDevice->write((const char*)pA32f->p_data + nWritten, nThisAudioBufferSize);
#if defined(MY_VERBOSE_LOGGING)
        qDebug() << "nWritten" << nWritten;
#endif
    } while (nWritten < nThisAudioBufferSize);
#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "-processAudio(...)";
#endif
}
