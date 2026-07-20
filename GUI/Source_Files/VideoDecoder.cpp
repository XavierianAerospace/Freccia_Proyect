#include "VideoDecoder.h"
#include <QDebug>
#include <QColor>

#ifdef HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#endif

VideoDecoder::VideoDecoder(QObject* parent)
    : QObject(parent), m_codecCtx(nullptr), m_frame(nullptr), m_rgbFrame(nullptr),
      m_swsCtx(nullptr), m_rgbBuffer(nullptr), m_bufferSize(0),
      m_currentWidth(0), m_currentHeight(0), m_currentPixFmt(-1) {
    initCodec();
}

VideoDecoder::~VideoDecoder() {
    cleanup();
}

void VideoDecoder::initCodec() {
#ifdef HAS_FFMPEG
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qWarning() << "H264 decoder not found!";
        return;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) return;

    // We don't open it here yet, we wait for initDecoder with params
    m_frame = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();
#endif
}

void VideoDecoder::initDecoder(AVCodecParameters* params) {
#ifdef HAS_FFMPEG
    if (!m_codecCtx || !params) return;

    qInfo() << "Initializing decoder with codec" << params->codec_id;

    // If codec is already open, we might need to close and reopen if params changed significantly
    // but for now let's just ensure it's configured.
    if (avcodec_parameters_to_context(m_codecCtx, params) < 0) {
        qWarning() << "Failed to copy codec parameters to context";
        return;
    }

    const AVCodec* codec = avcodec_find_decoder(m_codecCtx->codec_id);
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        qWarning() << "Failed to open codec";
        return;
    }
    qInfo() << "Decoder initialized successfully:" << m_codecCtx->width << "x" << m_codecCtx->height;
#endif
}

void VideoDecoder::decodePacket(AVPacket* packet) {
#ifdef HAS_FFMPEG
    if (!m_codecCtx || !packet) return;

    // Ensure codec is open (in case initDecoder wasn't called or failed)
    if (!avcodec_is_open(m_codecCtx)) {
        const AVCodec* codec = avcodec_find_decoder(m_codecCtx->codec_id);
        if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) return;
    }

    int ret = avcodec_send_packet(m_codecCtx, packet);
    if (ret < 0) return;

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        else if (ret < 0) return;

        // Detect resolution or format changes
        if (!m_swsCtx || m_frame->width != m_currentWidth || m_frame->height != m_currentHeight || m_frame->format != m_currentPixFmt) {

            qInfo() << "Video format changed or initialized:" << m_frame->width << "x" << m_frame->height << "Format:" << m_frame->format;

            m_swsCtx = sws_getCachedContext(m_swsCtx,
                                            m_frame->width, m_frame->height, (AVPixelFormat)m_frame->format,
                                            m_frame->width, m_frame->height, AV_PIX_FMT_RGB24,
                                            SWS_BILINEAR, nullptr, nullptr, nullptr);

            int newBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_frame->width, m_frame->height, 1);
            if (newBufferSize != m_bufferSize) {
                if (m_rgbBuffer) av_free(m_rgbBuffer);
                m_bufferSize = newBufferSize;
                m_rgbBuffer = (uint8_t*)av_malloc(m_bufferSize);
            }

            av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_rgbBuffer, AV_PIX_FMT_RGB24, m_frame->width, m_frame->height, 1);

            m_currentWidth = m_frame->width;
            m_currentHeight = m_frame->height;
            m_currentPixFmt = m_frame->format;
        }

        sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_frame->height, m_rgbFrame->data, m_rgbFrame->linesize);

        // RGB24 usually has 3 bytes per pixel. Line stride is important.
        QImage image(m_rgbBuffer, m_frame->width, m_frame->height, m_frame->width * 3, QImage::Format_RGB888);
        emit frameDecoded(image.copy());
    }
#endif
}

void VideoDecoder::decodeRawPacket(const QByteArray& data) {
    if (data.isEmpty()) return;

    static int packetCounter = 0;
    if (++packetCounter % 10 == 0) {
        static int frameCounter = 0;
        QImage mockFrame(640, 480, QImage::Format_RGB888);
        mockFrame.fill(QColor::fromHsv((frameCounter++) % 360, 200, 150));
        emit frameDecoded(mockFrame);
    }
}

void VideoDecoder::cleanup() {
#ifdef HAS_FFMPEG
    if (m_swsCtx) sws_freeContext(m_swsCtx);
    if (m_rgbBuffer) av_free(m_rgbBuffer);
    if (m_rgbFrame) av_frame_free(&m_rgbFrame);
    if (m_frame) av_frame_free(&m_frame);
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
#endif
}
