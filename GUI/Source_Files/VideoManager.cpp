#include "VideoManager.h"
#include <QNetworkDatagram>

VideoManager::VideoManager(quint16 port, QObject* parent)
    : QObject(parent), m_port(port), m_running(false) {
    m_udpSocket = new QUdpSocket(this);
}

VideoManager::~VideoManager() {
    stop();
}

void VideoManager::start() {
    if (m_running) return;

    m_udpSocket->bind(QHostAddress::Any, m_port);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &VideoManager::processPendingDatagrams);
    m_running = true;
}

void VideoManager::stop() {
    if (!m_running) return;
    m_udpSocket->close();
    m_running = false;
}

void VideoManager::processPendingDatagrams() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        emit packetReceived(datagram.data());
    }
}
