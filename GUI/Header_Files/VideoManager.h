#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QObject>
#include <QThread>

// Forward declarations for FFmpeg to avoid header dependency in H files
struct AVFormatContext;
struct AVPacket;
struct AVCodecParameters;

class VideoManager : public QObject {
    Q_OBJECT
public:
    explicit VideoManager(quint16 port = 5600, QObject* parent = nullptr);
    ~VideoManager();

public slots:
    void start();
    void stop();

signals:
    void packetReceived(AVPacket* packet);
    void codecParametersDetected(AVCodecParameters* params);
    void rawPacketReceived(const QByteArray& data);
    void connectionStatusChanged(bool connected);

private:
    void runReceptionLoop();
    void runMockLoop();

    quint16 m_port;
    bool m_running;
    AVFormatContext* m_formatCtx;
};

#endif // VIDEOMANAGER_H
