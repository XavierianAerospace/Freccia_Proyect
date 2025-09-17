#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include "SensorData.h"
#include "SerialReader.h"
#include "DataCleaner.h"

#include <vector>
#include <fstream>
#include <QObject>
#include <QString>

class SensorManager : public QObject {
    Q_OBJECT

public:
    explicit SensorManager(QObject* parent = nullptr);
    void processRawData(const QByteArray& line);
    ~SensorManager();

    const std::vector<SensorData>& getVectorData() const { return vectorData; }

    // Método para cerrar/abrir el puerto en caliente
public slots:
    bool setSerial(const QString& portName, int baud = 115200);

signals:
    void newSensorData(const SensorData& data);
    
    // Conexion con la UI para reconfigurar el puerto
    void serialReconfigured(QString port, int baud, bool ok);

private:
    SerialReader* m_serialReader = nullptr;
    std::vector<SensorData> vectorData;
    DataCleaner cleaner;

    // Guardar configuración actual del puerto
    QString currentPort_ = QString();
    int currentBaud_ = 115200;
};

#endif
