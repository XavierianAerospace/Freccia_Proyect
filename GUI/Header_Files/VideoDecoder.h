#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QImage>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoDecoder : public QObject {
    Q_OBJECT
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

public slots:
    void decodePacket(AVPacket* packet);

signals:
    void frameDecoded(const QImage& frame);

private:
    void initCodec();
    void cleanup();

    AVCodecContext* m_codecCtx;
    AVFrame* m_frame;
    AVFrame* m_rgbFrame;
    SwsContext* m_swsCtx;
    uint8_t* m_rgbBuffer;
    int m_bufferSize;
};

#endif // VIDEODECODER_H
