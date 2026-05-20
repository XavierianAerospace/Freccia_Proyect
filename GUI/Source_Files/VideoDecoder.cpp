#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(QObject* parent) : QObject(parent) {
    // Initialize FFmpeg here
}

VideoDecoder::~VideoDecoder() {
    // Cleanup FFmpeg here
}

void VideoDecoder::decodePacket(const QByteArray& data) {
    // Actual decoding logic (FFmpeg) goes here.
    // For now, we process packets to simulate live rendering.

    if (data.isEmpty()) return;

    // MOCK: Generate a static color frame or noise to prove rendering is working
    // In a real scenario, data is parsed for NAL units and sent to FFmpeg.
    static int frameCounter = 0;
    QImage mockFrame(640, 480, QImage::Format_RGB32);
    mockFrame.fill(QColor::fromHsv((frameCounter++) % 360, 150, 100));

    emit frameDecoded(mockFrame);
}
