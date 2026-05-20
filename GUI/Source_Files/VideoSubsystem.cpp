#include "VideoSubsystem.h"

VideoSubsystem* VideoSubsystem::m_instance = nullptr;

VideoSubsystem* VideoSubsystem::instance() {
    if (!m_instance) {
        m_instance = new VideoSubsystem();
    }
    return m_instance;
}

VideoSubsystem::VideoSubsystem(QObject* parent) : QObject(parent) {
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
    connect(vc->manager, &VideoManager::rawPacketReceived, vc->decoder, &VideoDecoder::decodeRawPacket);

    connect(vc->decoder, &VideoDecoder::frameDecoded, this, [=](const QImage& frame) {
        emit frameReady(camId, frame);
    });

    m_channels[camId] = vc;
    vc->thread->start();
}

void VideoSubsystem::stop(int camId) {
    if (!m_channels.contains(camId)) return;

    VideoChannel* vc = m_channels.take(camId);
    vc->thread->quit();
    vc->thread->wait();

    delete vc->manager;
    delete vc->decoder;
    delete vc->thread;
    delete vc;
}
