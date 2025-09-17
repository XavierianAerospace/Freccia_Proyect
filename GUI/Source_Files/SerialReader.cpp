#include "SerialReader.h"
#include <QtSerialPort/QSerialPortInfo>

SerialReader::SerialReader(QObject* parent)
    : QObject(parent),
      port_(new QSerialPort(this))
{
    connect(port_, &QSerialPort::readyRead, this, &SerialReader::onReadyRead);
}

bool SerialReader::start(const QString& portName, int baud) {
    if (port_->isOpen())
        port_->close();

    port_->setPortName(portName);
    port_->setBaudRate(baud);
    port_->setDataBits(QSerialPort::Data8);
    port_->setParity(QSerialPort::NoParity);
    port_->setStopBits(QSerialPort::OneStop);
    port_->setFlowControl(QSerialPort::NoFlowControl);

    if (!port_->open(QIODevice::ReadOnly)) {
        return false;
    }

    buffer_.clear();
    return true;
}

void SerialReader::stop() {
    if (port_->isOpen())
        port_->close();
}

void SerialReader::onReadyRead() {
    buffer_.append(port_->readAll());

    int idx = -1;
    while ((idx = buffer_.indexOf('\n')) != -1) {
        QByteArray line = buffer_.left(idx + 1);
        buffer_.remove(0, idx + 1);
        emit dataReceived(line); 
    }
}
