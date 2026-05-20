#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(QObject* parent) : QObject(parent) {
    // Initialize FFmpeg here
}

VideoDecoder::~VideoDecoder() {
    // Cleanup FFmpeg here
}

void VideoDecoder::decodePacket(const QByteArray& data) {
    /**
     * CORRECTION: H264 is NOT an image.
     * PIPELINE: UDP -> MPEGTS -> H264 Decoder -> YUV -> RGB -> Render
     */

    if (data.isEmpty()) return;

    // Optimization: Only process enough data to simulate frame boundaries.
    // In real H264, we search for NAL start codes: 00 00 00 01.

    static int packetCounter = 0;
    packetCounter++;

    // MOCK: Emit a frame approximately every 10 packets to simulate 30-60 FPS
    // depending on network throughput, instead of every UDP datagram.
    if (packetCounter % 10 == 0) {
        static int frameCounter = 0;
        QImage mockFrame(640, 480, QImage::Format_RGB32);

        // Simulate high-speed asynchronous frame delivery with rotating colors
        mockFrame.fill(QColor::fromHsv((frameCounter++) % 360, 200, 150));

        emit frameDecoded(mockFrame);
    }
}
