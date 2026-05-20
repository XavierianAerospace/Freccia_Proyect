#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(QObject* parent)
    : QObject(parent), m_codecCtx(nullptr), m_frame(nullptr), m_rgbFrame(nullptr),
      m_swsCtx(nullptr), m_rgbBuffer(nullptr) {
    initCodec();
}

VideoDecoder::~VideoDecoder() {
    cleanup();
}

void VideoDecoder::initCodec() {
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) return;

    m_codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) return;

    m_frame = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();
}

void VideoDecoder::decodePacket(AVPacket* packet) {
    if (!m_codecCtx || !packet) return;

    int ret = avcodec_send_packet(m_codecCtx, packet);
    if (ret < 0) return;

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        else if (ret < 0) return;

        // Perform YUV420P -> RGB24 conversion
        if (!m_swsCtx) {
            m_swsCtx = sws_getContext(m_frame->width, m_frame->height, (AVPixelFormat)m_frame->format,
                                       m_frame->width, m_frame->height, AV_PIX_FMT_RGB24,
                                       SWS_BILINEAR, nullptr, nullptr, nullptr);

            m_bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_frame->width, m_frame->height, 1);
            m_rgbBuffer = (uint8_t*)av_malloc(m_bufferSize);
            av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_rgbBuffer, AV_PIX_FMT_RGB24, m_frame->width, m_frame->height, 1);
        }

        sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_frame->height, m_rgbFrame->data, m_rgbFrame->linesize);

        // Convert to QImage and emit
        QImage image(m_rgbBuffer, m_frame->width, m_frame->height, QImage::Format_RGB888);
        emit frameDecoded(image.copy()); // Deep copy for safe thread emission
    }
}

void VideoDecoder::cleanup() {
    if (m_swsCtx) sws_freeContext(m_swsCtx);
    if (m_rgbBuffer) av_free(m_rgbBuffer);
    if (m_rgbFrame) av_frame_free(&m_rgbFrame);
    if (m_frame) av_frame_free(&m_frame);
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
}
