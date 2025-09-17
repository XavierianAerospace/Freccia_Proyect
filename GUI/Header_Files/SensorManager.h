#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include "SensorData.h"
#include "SerialReader.h"
#include "DataCleaner.h"

#include <vector>
#include <QObject>
#include <QString>

class SensorManager : public QObject {
    Q_OBJECT

public:
    explicit SensorManager(QObject* parent = nullptr);
    ~SensorManager();

    // Entrada cruda desde SerialReader
    void processRawData(const QByteArray& line);

    // Acceso a histórico (si lo usas en otra parte)
    const std::vector<SensorData>& getVectorData() const { return vectorData; }

public slots:
    // Reconfigurar puerto/baud en caliente
    bool setSerial(const QString& portName, int baud = 115200);
    void clearData();

    // Habilitar/deshabilitar recepción (procesamiento/guardado/emisión)
    void setReceivingEnabled(bool enabled) { receivingEnabled_ = enabled; }

    bool loadFromCsv(const QString& path);

signals:
    void newSensorData(const SensorData& data);

    // UI: notificar reconfiguración del puerto
    void serialReconfigured(QString port, int baud, bool ok);

private:
    SerialReader* m_serialReader = nullptr;
    std::vector<SensorData> vectorData;
    DataCleaner cleaner;

    // Config actual del puerto
    QString currentPort_ = QString();
    int     currentBaud_ = 115200;

    // Flag para ignorar completamente los datos entrantes
    bool receivingEnabled_ = true;
};

#endif // SENSORMANAGER_H
