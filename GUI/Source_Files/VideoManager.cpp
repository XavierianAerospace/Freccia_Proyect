#include "VideoManager.h"
#include <QDebug>
#include <QUdpSocket>
#include <QNetworkDatagram>

#ifdef HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#endif

VideoManager::VideoManager(quint16 port, QObject* parent)
    : QObject(parent), m_port(port), m_running(false), m_formatCtx(nullptr) {
}

VideoManager::~VideoManager() {
    stop();
}

void VideoManager::start() {
    if (m_running) return;
    m_running = true;

#ifdef HAS_FFMPEG
    runReceptionLoop();
#else
    runMockLoop();
#endif
}

void VideoManager::stop() {
    m_running = false;
}

void VideoManager::runReceptionLoop() {
#ifdef HAS_FFMPEG
    qInfo() << "Starting VideoManager FFmpeg reception loop on port" << m_port;
    QString url = QString("udp://0.0.0.0:%1").arg(m_port);

    while (m_running) {
        AVDictionary* options = nullptr;
        av_dict_set(&options, "buffer_size", "1048576", 0); // 1M bufsize
        av_dict_set(&options, "fifo_size", "10000", 0);
        av_dict_set(&options, "overrun_nonfatal", "1", 0);
        av_dict_set(&options, "stimeout", "3000000", 0); // 3s timeout

        m_formatCtx = nullptr;
        if (avformat_open_input(&m_formatCtx, url.toStdString().c_str(), nullptr, &options) < 0) {
            av_dict_free(&options);
            qWarning() << "Could not open video stream:" << url << ". Retrying in 2 seconds...";
            emit connectionStatusChanged(false);
            QThread::sleep(2);
            continue;
        }

        if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
            qWarning() << "Could not find stream information for" << url << ". Retrying...";
            avformat_close_input(&m_formatCtx);
            av_dict_free(&options);
            QThread::sleep(1);
            continue;
        }

        av_dict_free(&options); // Free if not consumed or partially consumed

        int videoStreamIndex = -1;
        for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
            if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1) {
            qWarning() << "Could not find video stream in" << url;
            avformat_close_input(&m_formatCtx);
            QThread::sleep(1);
            continue;
        }

        // Notify about the codec parameters
        emit codecParametersDetected(m_formatCtx->streams[videoStreamIndex]->codecpar);
        emit connectionStatusChanged(true);

        AVPacket* packet = av_packet_alloc();
        while (m_running) {
            if (av_read_frame(m_formatCtx, packet) >= 0) {
                if (packet->stream_index == videoStreamIndex) {
                    emit packetReceived(packet);
                }
                av_packet_unref(packet);
            } else {
                qWarning() << "Stream read error on" << url << ". Attempting reconnect...";
                emit connectionStatusChanged(false);
                break;
            }
            // Small sleep to avoid CPU saturation if reading is too fast
            QThread::msleep(1);
        }

        av_packet_free(&packet);
        avformat_close_input(&m_formatCtx);
    }
#else
    qWarning() << "FFmpeg NOT enabled. VideoManager will NOT receive real video.";
#endif
}

void VideoManager::runMockLoop() {
    QUdpSocket socket;
    socket.bind(QHostAddress::Any, m_port);
    emit connectionStatusChanged(true);

    while (m_running) {
        while (socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = socket.receiveDatagram();
            emit rawPacketReceived(datagram.data());
        }
        QThread::msleep(10);
    }
}
