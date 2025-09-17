#include "SensorManager.h"
#include "FileHelper.h"

#include <QStringList>
#include <QTimer>

SensorManager::SensorManager(QObject* parent) : QObject(parent) {
    m_serialReader = new SerialReader(this);
    connect(m_serialReader, &SerialReader::dataReceived,
            this, &SensorManager::processRawData);

#ifdef Q_OS_WIN
    currentPort_ = "COM3";
#else
    currentPort_ = "/dev/pts/3";
#endif
    currentBaud_ = 115200;

    QTimer::singleShot(200, this, [this]() {
        m_serialReader->start(currentPort_, currentBaud_);
    });
}

bool SensorManager::setSerial(const QString& portName, int baud) {
    bool ok = false;
    try {
        if (m_serialReader) m_serialReader->stop();
        ok = (m_serialReader && m_serialReader->start(portName, baud));
        if (ok) {
            currentPort_ = portName;
            currentBaud_ = baud;
        }
    } catch (...) {
        ok = false;
    }
    emit serialReconfigured(portName, baud, ok);
    return ok;
}

void SensorManager::processRawData(const QByteArray& line) {
    if (!receivingEnabled_) return;

    const QString str = QString::fromUtf8(line).trimmed();
    if (str.isEmpty()) return;

    const QStringList values = str.split(',');
    if (values.size() < 15) return;

    try {
        SensorData sensor;
        sensor.latitude    = values[0].toDouble();
        sensor.longitude   = values[1].toDouble();
        sensor.date        = values[2].toStdString();
        sensor.utc_time    = values[3].toStdString();
        sensor.secs        = values[4].toFloat();
        sensor.satellites  = values[5].toInt();
        sensor.hdop        = values[6].toFloat();
        sensor.Roll        = values[7].toFloat();
        sensor.Pitch       = values[8].toFloat();
        sensor.Yaw         = values[9].toFloat();
        sensor.Servo1      = values[10].toFloat();
        sensor.Servo2      = values[11].toFloat();
        sensor.Servo3      = values[12].toFloat();
        sensor.Servo4      = values[13].toFloat();
        sensor.AltDiff     = values[14].toFloat();

        sensor.pressure    = 0.0f;
        sensor.temperature = 0.0f;

        vectorData.push_back(sensor);

        FileHelper::appendRawData(std::string("../data/raw_data.csv"), sensor);

        cleaner.clean(vectorData);

        emit newSensorData(sensor);
    } catch (...) {
        // Ignorar errores de conversión/parsing
    }
}

void SensorManager::clearData() {
    vectorData.clear();
    emit newSensorData(SensorData{});
}

SensorManager::~SensorManager() {}
