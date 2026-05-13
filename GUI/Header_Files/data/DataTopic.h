#ifndef DATATOPIC_H
#define DATATOPIC_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class DataTopic : public QObject {
    Q_OBJECT
public:
    static DataTopic* instance();

    void publish(const QString& data);

signals:
    void dataPublished(const QString& data);

private slots:
    void onNewConnection();
    void onClientDisconnected();

private:
    explicit DataTopic(QObject* parent = nullptr);
    ~DataTopic();

    static DataTopic* m_instance;
    QTcpServer* m_tcpServer;
    QList<QTcpSocket*> m_clients;
};

#endif // DATATOPIC_H
