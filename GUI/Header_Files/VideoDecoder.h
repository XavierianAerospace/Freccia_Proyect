#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QImage>

// Forward declarations for FFmpeg
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct AVCodecParameters;

class VideoDecoder : public QObject {
    Q_OBJECT
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

public slots:
    void decodePacket(AVPacket* packet);
    void decodeRawPacket(const QByteArray& data);
    void initDecoder(AVCodecParameters* params);

signals:
    void frameDecoded(const QImage& frame);

private:
    void initCodec();
    void cleanup();

    AVCodecContext* m_codecCtx;
    AVFrame* m_frame;
    AVFrame* m_rgbFrame;
    SwsContext* m_swsCtx;
    unsigned char* m_rgbBuffer;
    int m_bufferSize;

    int m_currentWidth;
    int m_currentHeight;
    int m_currentPixFmt;
};

#endif // VIDEODECODER_H
