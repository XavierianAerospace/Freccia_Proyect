#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QObject>
#include <QThread>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

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
    void connectionStatusChanged(bool connected);

private:
    void runReceptionLoop();

    quint16 m_port;
    bool m_running;
    AVFormatContext* m_formatCtx;
};

#endif // VIDEOMANAGER_H
