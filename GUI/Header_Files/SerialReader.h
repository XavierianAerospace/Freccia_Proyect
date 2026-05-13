#ifndef SERIALREADER_H
#define SERIALREADER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>

class SerialReader : public QObject {
    Q_OBJECT
public:
    explicit SerialReader(QObject* parent = nullptr);

    bool start(const QString& portName, int baud = 115200);

    bool start(const QString& portName) { return start(portName, 115200); }

    // Cierra el puerto si está abierto
    void stop();

signals:
    void dataReceived(const QByteArray& line);

private slots:
    void onReadyRead();

private:
    QSerialPort* port_ = nullptr;
    QByteArray buffer_;
};

#endif // SERIALREADER_H
