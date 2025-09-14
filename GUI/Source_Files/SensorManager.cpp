#include "SensorManager.h"
#include "FileHelper.h" 
#include <QStringList>
#include <QTimer>

SensorManager::SensorManager(QObject* parent) : QObject(parent) {
    m_serialReader = new SerialReader(this);
    connect(m_serialReader, &SerialReader::dataReceived, this, &SensorManager::processRawData);

    // Iniciar lectura ligeramente después para evitar problemas de sincronización
    QTimer::singleShot(200, this, [this]() {
        m_serialReader->start("COM3");
    });
}

void SensorManager::processRawData(const QByteArray& line) {
    QString str = QString::fromUtf8(line).trimmed();
    QStringList values = str.split(',');

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

        // Guarda histórico en memoria si lo necesitas
        vectorData.push_back(sensor);

        // ⬇️ Escribe inmediatamente UN registro en raw_data.csv (con timestamp y header si falta)
        FileHelper::appendRawData("../data/raw_data.csv", sensor);

        // Limpieza y almacenamiento limpio / de errores
        cleaner.clean(vectorData);

        emit newSensorData(sensor);
    } catch (...) {
        // Ignorar errores de conversión
    }
}

SensorManager::~SensorManager() {}
