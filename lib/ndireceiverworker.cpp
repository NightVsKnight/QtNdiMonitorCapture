#include "ndireceiverworker.h"

#include <QDateTime>
#include <QPainter>
#include <QThread>

#include "ndiwrapper.h"

NdiReceiverWorker::NdiReceiverWorker(QObject* pParent)
    : QObject(pParent)
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
    m_connectionMetadata = nullptr;
    m_bIsRunning = false;
    m_selectedSourceName = nullptr;
    m_bMuteAudio = false;
    m_cIDX = nullptr;
    qDebug() << "-init()";
}

void NdiReceiverWorker::setConnectionInfo(const QString& receiverName, const QString& connectionMetadata)
{
    m_receiverName = receiverName;
    m_connectionMetadata = connectionMetadata;
    m_bReconnect = true;
}

QString NdiReceiverWorker::selectedSourceName()
{
    return m_selectedSourceName;
}

void NdiReceiverWorker::selectSource(const QString& sourceName)
{
    m_selectedSourceName = sourceName;
}

void NdiReceiverWorker::muteAudio(bool bMute)
{
    m_bMuteAudio = bMute;
}

void NdiReceiverWorker::sendMetadata(const QString& metadata)
{
    if (!m_bIsRunning) return;
    m_listMetadatasToSend.append(metadata);
}

void NdiReceiverWorker::stop()
{
    qDebug() << "+stop()";
    m_bIsRunning = false;
    qDebug() << "-stop()";
}

void NdiReceiverWorker::run()
{
    qDebug() << "+run()";

    if (m_bIsRunning) return;
    m_bIsRunning = true;

    QAudioSink* pAudioOutputSink = nullptr;
    QIODevice* pAudioIoDevice = nullptr;

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

    QString                     selectedSourceName;
    int64_t                     nVTimestamp = 0;
    int64_t                     nATimestamp = 0;
    NDIlib_recv_instance_t      pNdiRecv = nullptr;
    NDIlib_framesync_instance_t pNdiFrameSync = nullptr;

    auto pNdi = NdiWrapper::ndiGet();

    QString metadataSend;

    bool isConnected = false;

    while (m_bIsRunning)
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

        if (m_bReconnect || (selectedSourceName != m_selectedSourceName))
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

            auto ndiSources = NdiWrapper::ndiFindSources();
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
                auto _selectedSourceName = QString::fromUtf8(ndiList.at(i).p_ndi_name);
                if (QString::compare(_selectedSourceName, m_selectedSourceName, Qt::CaseInsensitive) == 0)
                {
                    nSourceChannel = i;
                    selectedSourceName = _selectedSourceName;
                    break;
                }
            }
            if (nSourceChannel == -1)
            {
                qDebug() << "Selected NDI source not available; continue;";
                if (isConnected)
                {
                    isConnected = false;
                    emit onSourceConnected(selectedSourceName);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (true)
            {
                qDebug() << "NDI source activated: " << selectedSourceName;
            }

            if (true)
            {
                qDebug() << "NDI init starts...";
            }

            if (pNdiFrameSync != nullptr)
            {
                pNdi->framesync_destroy(pNdiFrameSync);
                pNdiFrameSync = nullptr;
            }

            if (pNdiRecv != nullptr)
            {
                if (pAudioOutputSink && m_bIsRunning)
                {
                    if (true)
                    {
                        qDebug() << "NDI audio stop.";
                    }
                    pAudioOutputSink->stop();
                    pAudioIoDevice = nullptr;
                }

                pNdi->recv_destroy(pNdiRecv);
                pNdiRecv = nullptr;
            }

            NDIlib_recv_create_v3_t recv_desc;
            auto utf8 = m_receiverName.toUtf8();
            recv_desc.p_ndi_recv_name = utf8.constData();
            recv_desc.source_to_connect_to = ndiList.at(nSourceChannel);
            //recv_desc.color_format = NDIlib_recv_color_format_UYVY_BGRA;
            //recv_desc.color_format = NDIlib_recv_color_format_BGRX_BGRA;
            recv_desc.color_format = NDIlib_recv_color_format_best;
            recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
            recv_desc.allow_video_fields = true;
            if (true)
            {
                qDebug().nospace() << "NDI_recv_create_desc.source_to_connect_to=" << QString(recv_desc.source_to_connect_to.p_ndi_name);
                qDebug().nospace() << "NDI_recv_create_desc.color_format=" << recv_desc.color_format;
                qDebug().nospace() << "NDI_recv_create_desc.bandwidth=" << recv_desc.bandwidth;
            }

            qDebug() << "pNdi->recv_create_v3";
            pNdiRecv = pNdi->recv_create_v3(&recv_desc);
            if (pNdiRecv == nullptr)
            {
                qDebug() << "pNdi->recv_create_v3 failed; should probably throw an exception here...";
                break;
            }

            nVTimestamp = 0;
            nATimestamp = 0;

            qDebug() << "pNdi->framesync_create";
            pNdiFrameSync = pNdi->framesync_create(pNdiRecv);
            if (pNdiFrameSync == nullptr)
            {
                qDebug() << "pNdi->framesync_create failed; should probably throw an exception here...";
                break;
            }

            pNdi->recv_clear_connection_metadata(pNdiRecv);

            NDIlib_metadata_frame_t enable_hw_accel;
            enable_hw_accel.p_data = (char*)"<ndi_video_codec type=\"hardware\"/>";
            qDebug() << "pNdi->recv_send_metadata enable_hw_accel" << enable_hw_accel.p_data;
            pNdi->recv_send_metadata(pNdiRecv, &enable_hw_accel);

            if (!m_connectionMetadata.isEmpty())
            {
                NDIlib_metadata_frame_t connection_metadata;
                auto utf8 = m_connectionMetadata.toUtf8();
                connection_metadata.p_data = (char*)utf8.constData();
                qDebug() << "pNdi->recv_add_connection_metadata connection_metadata" << connection_metadata.p_data;
                pNdi->recv_add_connection_metadata(pNdiRecv, &connection_metadata);
            }

            //cfg->setRecStatus(1);

            if (pAudioOutputSink && m_bIsRunning)
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

        if (selectedSourceName.isEmpty())
        {
            qDebug() << "No NDI source selected; continue";
            if (isConnected)
            {
                isConnected = false;
                emit onSourceDisconnected(selectedSourceName);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        else
        {
            //if ()
        }

        if (pNdi->recv_get_no_connections(pNdiRecv) == 0)
        {
            if (isConnected)
            {
                isConnected = false;
                emit onSourceDisconnected(selectedSourceName);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        if (!isConnected)
        {
            isConnected = true;
            emit onSourceConnected(selectedSourceName);
        }

        //cNdiSourceName = m_cNdiSourceName;

        //
        // TODO: CLEAN UP THIS MESS: END!
        //

        //
        // VIDEO
        //
        video_frame = {};
        //qDebug() << "pNdi->framesync_capture_video";
        pNdi->framesync_capture_video(pNdiFrameSync, &video_frame, NDIlib_frame_format_type_progressive);
        //qDebug() << "video_frame.p_data=" << video_frame.p_data;
        if (video_frame.p_data && (video_frame.timestamp > nVTimestamp))
        {
            //qDebug() << "v";//ideo_frame";
            nVTimestamp = video_frame.timestamp;
            processVideo(video_frame);
        }
        pNdi->framesync_free_video(pNdiFrameSync, &video_frame);

        //
        // AUDIO
        //
        audio_frame = {};
        //qDebug() << "pNdi->framesync_capture_audio";
        pNdi->framesync_capture_audio(pNdiFrameSync, &audio_frame, audioFormat.sampleRate(), audioFormat.channelCount(), 1024);
        //qDebug() << "audio_frame.p_data=" << audio_frame.p_data;
        if (audio_frame.p_data && (audio_frame.timestamp > nATimestamp))
        {
            //qDebug() << "a";//udio_frame";
            nATimestamp = audio_frame.timestamp;
            processAudio(pNdi, &audio_frame, &a32f, &nAudioBufferSize, pAudioIoDevice);
        }
        pNdi->framesync_free_audio(pNdiFrameSync, &audio_frame);

        //
        // METADATA RECV
        //
        switch(pNdi->recv_capture_v2(pNdiRecv, nullptr, nullptr, &metadata_frame, 0))
        {
        case NDIlib_frame_type_e::NDIlib_frame_type_metadata:
        {
            //qDebug() << "m";//"etadata_frame";
            auto metadata = QString::fromUtf8(metadata_frame.p_data);
            pNdi->recv_free_metadata(pNdiRecv, &metadata_frame);
            qDebug() << "pNdi->recv_capture_v2 NDIlib_frame_type_metadata" << metadata;
            emit onMetadataReceived(metadata);
            break;
        }
        case NDIlib_frame_type_e::NDIlib_frame_type_status_change:
            qDebug() << "pNdi->recv_capture_v2 NDIlib_frame_type_status_change";
            break;
        default:
            // ignore
            break;
        }

        //
        // METADATA SEND
        //
        if (!m_listMetadatasToSend.isEmpty())
        {
            metadataSend = m_listMetadatasToSend.takeFirst();
            NDIlib_metadata_frame_t metadata_frame;
            auto utf8 = metadataSend.toUtf8();
            metadata_frame.p_data = (char*)utf8.constData();
            qDebug() << "pNdi->recv_send_metadata" << metadataSend;
            pNdi->recv_send_metadata(pNdiRecv, &metadata_frame);
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
        pNdi->framesync_destroy(pNdiFrameSync);
        pNdiFrameSync = nullptr;
    }

    if (pNdiRecv)
    {
        pNdi->recv_destroy(pNdiRecv);
        pNdiRecv = nullptr;
    }

    if (pAudioOutputSink)
    {
        pAudioOutputSink->stop();
        delete pAudioOutputSink;
        pAudioOutputSink = nullptr;
        pAudioIoDevice = nullptr;
    }

    qDebug() << "-run()";
}

//#define MY_VERBOSE_LOGGING

QString withCommas(int value)
{
    return QLocale(QLocale::English).toString(value);
}

void NdiReceiverWorker::processVideo(const NDIlib_video_frame_v2_t& pVideoFrameNdi)
{
    auto ndiWidth = pVideoFrameNdi.xres;
    auto ndiHeight = pVideoFrameNdi.yres;
    auto ndiLineStrideInBytes = pVideoFrameNdi.line_stride_in_bytes;
    auto ndiPixelFormat = pVideoFrameNdi.FourCC;
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
    auto pixelFormat = NdiWrapper::ndiPixelFormatToQtPixelFormat(ndiPixelFormat);
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
    auto pSrcY = pVideoFrameNdi.p_data;
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

    emit onVideoFrameReceived(videoFrame);

#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "-processVideo(...)";
#endif
}

void NdiReceiverWorker::processAudio(
        const NDIlib_v5* pNdi,
        NDIlib_audio_frame_v2_t* pAudioFrameNdi,
        NDIlib_audio_frame_interleaved_32f_t* pA32f,
        size_t* pnAudioBufferSize,
        QIODevice* pAudioIoDevice)
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

    size_t nThisAudioBufferSize = pAudioFrameNdi->no_samples * pAudioFrameNdi->no_channels * sizeof(float);
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

    pNdi->util_audio_to_interleaved_32f_v2(pAudioFrameNdi, pA32f);

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
