#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QImage>

/**
 * Wrapper for Video Decoding.
 * Note: Actual FFmpeg integration requires linking against libavcodec, libavformat, etc.
 * This class provides the structure to receive NAL units and emit QImages.
 */
class VideoDecoder : public QObject {
    Q_OBJECT
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

public slots:
    void decodePacket(const QByteArray& data);

signals:
    void frameDecoded(const QImage& frame);

private:
    // FFmpeg context pointers would go here
    // AVCodecContext *m_codecCtx;
    // AVFrame *m_frame;
    // ...
};

#endif // VIDEODECODER_H
