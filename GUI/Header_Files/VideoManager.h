#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QThread>

class VideoManager : public QObject {
    Q_OBJECT
public:
    explicit VideoManager(quint16 port = 5600, QObject* parent = nullptr);
    ~VideoManager();

    void start();
    void stop();

signals:
    void packetReceived(const QByteArray& data);

private slots:
    void processPendingDatagrams();

private:
    QUdpSocket* m_udpSocket;
    quint16 m_port;
    bool m_running;
};

#endif // VIDEOMANAGER_H
