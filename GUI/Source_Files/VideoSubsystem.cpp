#include "VideoSubsystem.h"

#ifdef HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
}
#endif

VideoSubsystem* VideoSubsystem::m_instance = nullptr;

VideoSubsystem* VideoSubsystem::instance() {
    if (!m_instance) {
        m_instance = new VideoSubsystem();
    }
    return m_instance;
}

VideoSubsystem::VideoSubsystem(QObject* parent) : QObject(parent) {
#ifdef HAS_FFMPEG
    avformat_network_init();
#endif
}

VideoSubsystem::~VideoSubsystem() {
    for (int id : m_channels.keys()) {
        stop(id);
    }
}

void VideoSubsystem::start(int camId, quint16 port) {
    if (m_channels.contains(camId)) return;

    VideoChannel* vc = new VideoChannel();
    vc->thread = new QThread(this);
    vc->manager = new VideoManager(port);
    vc->decoder = new VideoDecoder();

    vc->manager->moveToThread(vc->thread);
    vc->decoder->moveToThread(vc->thread);

    connect(vc->thread, &QThread::started, vc->manager, &VideoManager::start);
    connect(vc->manager, &VideoManager::packetReceived, vc->decoder, &VideoDecoder::decodePacket);
    connect(vc->manager, &VideoManager::codecParametersDetected, vc->decoder, &VideoDecoder::initDecoder);
    connect(vc->manager, &VideoManager::rawPacketReceived, vc->decoder, &VideoDecoder::decodeRawPacket);
    connect(vc->manager, &VideoManager::frameReceived, this, [=](const QImage& frame) {
        emit frameReady(camId, frame);
    });

    connect(vc->decoder, &VideoDecoder::frameDecoded, this, [=](const QImage& frame) {
        emit frameReady(camId, frame);
    });

    m_channels[camId] = vc;
    vc->thread->start();
}

void VideoSubsystem::stop(int camId) {
    if (!m_channels.contains(camId)) return;

    VideoChannel* vc = m_channels.take(camId);
    vc->manager->stop();
    vc->thread->quit();
    vc->thread->wait();

    delete vc->manager;
    delete vc->decoder;
    delete vc->thread;
    delete vc;
}
