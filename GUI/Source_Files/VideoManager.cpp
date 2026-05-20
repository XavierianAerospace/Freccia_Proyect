#include "VideoManager.h"
#include <QDebug>

VideoManager::VideoManager(quint16 port, QObject* parent)
    : QObject(parent), m_port(port), m_running(false), m_formatCtx(nullptr) {
}

VideoManager::~VideoManager() {
    stop();
}

void VideoManager::start() {
    if (m_running) return;
    m_running = true;
    runReceptionLoop();
}

void VideoManager::stop() {
    m_running = false;
}

void VideoManager::runReceptionLoop() {
    QString url = QString("udp://0.0.0.0:%1").arg(m_port);

    AVDictionary* options = nullptr;
    av_dict_set(&options, "buffer_size", "1048576", 0); // 1M bufsize
    av_dict_set(&options, "fifo_size", "10000", 0);
    av_dict_set(&options, "overrun_nonfatal", "1", 0);
    av_dict_set(&options, "stimeout", "3000000", 0); // 3s timeout

    while (m_running) {
        if (avformat_open_input(&m_formatCtx, url.toStdString().c_str(), nullptr, &options) < 0) {
            qWarning() << "Could not open video stream:" << url << ". Retrying in 2 seconds...";
            emit connectionStatusChanged(false);
            QThread::sleep(2);
            continue;
        }

        if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
            qWarning() << "Could not find stream information. Retrying...";
            avformat_close_input(&m_formatCtx);
            QThread::sleep(1);
            continue;
        }

        emit connectionStatusChanged(true);

        AVPacket* packet = av_packet_alloc();
        while (m_running) {
            if (av_read_frame(m_formatCtx, packet) >= 0) {
                emit packetReceived(packet);
                av_packet_unref(packet);
            } else {
                // Read error or timeout, break inner loop to attempt reconnect
                qWarning() << "Stream read error. Attempting reconnect...";
                emit connectionStatusChanged(false);
                break;
            }
            QThread::msleep(1);
        }

        av_packet_free(&packet);
        avformat_close_input(&m_formatCtx);
    }
}
