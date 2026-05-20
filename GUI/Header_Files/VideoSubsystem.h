#ifndef VIDEOSUBSYSTEM_H
#define VIDEOSUBSYSTEM_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMap>
#include "VideoManager.h"
#include "VideoDecoder.h"

class VideoSubsystem : public QObject {
    Q_OBJECT
public:
    static VideoSubsystem* instance();

    void start(int camId, quint16 port);
    void stop(int camId);

signals:
    void frameReady(int camId, const QImage& frame);

private:
    explicit VideoSubsystem(QObject* parent = nullptr);
    ~VideoSubsystem();

    static VideoSubsystem* m_instance;

    struct VideoChannel {
        VideoManager* manager;
        VideoDecoder* decoder;
        QThread* thread;
    };

    QMap<int, VideoChannel*> m_channels;
};

#endif // VIDEOSUBSYSTEM_H
