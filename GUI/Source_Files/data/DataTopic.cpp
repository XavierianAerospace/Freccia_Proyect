#include "DataTopic.h"
#include <QDebug>

DataTopic* DataTopic::m_instance = nullptr;

DataTopic* DataTopic::instance() {
    if (!m_instance) {
        m_instance = new DataTopic();
    }
    return m_instance;
}

DataTopic::DataTopic(QObject* parent) : QObject(parent) {
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &DataTopic::onNewConnection);

    if (!m_tcpServer->listen(QHostAddress::Any, 5000)) {
        qWarning() << "DataTopic: No se pudo iniciar servidor TCP en puerto 5000";
    } else {
        qDebug() << "DataTopic: Servidor TCP escuchando en puerto 5000";
    }
}

DataTopic::~DataTopic() {
    for (QTcpSocket* client : m_clients) {
        client->disconnectFromHost();
    }
}

void DataTopic::publish(const QString& data) {
    emit dataPublished(data);

    QByteArray msg = (data + "\n").toUtf8();
    for (QTcpSocket* client : m_clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(msg);
            client->flush();
        }
    }
}

void DataTopic::onNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* client = m_tcpServer->nextPendingConnection();
        m_clients << client;
        connect(client, &QTcpSocket::disconnected, this, &DataTopic::onClientDisconnected);
        qDebug() << "DataTopic: Nuevo cliente conectado";
    }
}

void DataTopic::onClientDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
        qDebug() << "DataTopic: Cliente desconectado";
    }
}
