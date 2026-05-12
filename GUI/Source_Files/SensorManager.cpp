#include "SensorManager.h"
#include "FileHelper.h"
#include "data/DataTopic.h"

#include <QStringList>
#include <QTimer>

#include <QFile>
#include <QTextStream>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  #include <QStringConverter>
#endif

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

        // Limpieza y corrección de datos
        std::vector<SensorData> tempVec = {sensor};
        cleaner.clean(tempVec);
        sensor = tempVec.back();

        vectorData.push_back(sensor);

        FileHelper::appendRawData(std::string("../data/raw_data.csv"), sensor);

        // Publicar a través de DataTopic
        DataTopic::instance()->publish(sensor.serialize());

    } catch (...) {
        // Ignorar errores de conversión/parsing
    }
}

void SensorManager::clearData() {
    vectorData.clear();
    DataTopic::instance()->publish(SensorData{}.serialize());
}

bool SensorManager::loadFromCsv(const QString& path)
{
    // Detén la recepción en vivo mientras cargas
    receivingEnabled_ = false;

    // Limpia el buffer actual
    vectorData.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#endif

    bool firstLine = true;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // Salta encabezado
        if (firstLine) {
            firstLine = false;
            if (line.startsWith("Hora,"))
                continue;
        }

        const QStringList v = line.split(',', Qt::KeepEmptyParts);
        if (v.size() < 18)
            continue; // línea incompleta

        SensorData s{};
        s.latitude    = v[1].toDouble();
        s.longitude   = v[2].toDouble();
        s.date        = v[3].toStdString();   // Fecha (la primera de la sesión)
        s.utc_time    = v[4].toStdString();   // Hora completa "hh:mm:ss.zzz"
        s.secs        = v[5].toFloat();       // segundos relativos
        s.satellites  = v[6].toInt();
        s.hdop        = v[7].toFloat();
        s.Roll        = v[8].toFloat();
        s.Pitch       = v[9].toFloat();
        s.Yaw         = v[10].toFloat();
        s.Servo1      = v[11].toFloat();
        s.Servo2      = v[12].toFloat();
        s.Servo3      = v[13].toFloat();
        s.Servo4      = v[14].toFloat();
        s.AltDiff     = v[15].toFloat();
        s.pressure    = v[16].toFloat();
        s.temperature = v[17].toFloat();

        vectorData.push_back(s);

        // Publicar a través de DataTopic
        DataTopic::instance()->publish(s.serialize());
    }

    f.close();

    return true;
}

SensorManager::~SensorManager() {}
