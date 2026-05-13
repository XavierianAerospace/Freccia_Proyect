#ifndef SENSORDATA_H
#define SENSORDATA_H

#include <string>
#include <QString>
#include <QStringList>

struct SensorData {
    double latitude;
    double longitude;
    std::string date;
    std::string utc_time;
    float secs;
    int satellites;
    float hdop;
    float Roll;
    float Pitch;
    float Yaw;
    float Servo1;
    float Servo2;
    float Servo3;
    float Servo4;
    float AltDiff;
    float pressure;
    float temperature;

    QString serialize() const {
        return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17")
            .arg(latitude, 0, 'f', 8)
            .arg(longitude, 0, 'f', 8)
            .arg(QString::fromStdString(date))
            .arg(QString::fromStdString(utc_time))
            .arg(secs)
            .arg(satellites)
            .arg(hdop)
            .arg(Roll)
            .arg(Pitch)
            .arg(Yaw)
            .arg(Servo1)
            .arg(Servo2)
            .arg(Servo3)
            .arg(Servo4)
            .arg(AltDiff)
            .arg(pressure)
            .arg(temperature);
    }

    static SensorData deserialize(const QString& line) {
        QStringList v = line.split(',');
        SensorData s{};
        if (v.size() >= 15) {
            s.latitude    = v[0].toDouble();
            s.longitude   = v[1].toDouble();
            s.date        = v[2].toStdString();
            s.utc_time    = v[3].toStdString();
            s.secs        = v[4].toFloat();
            s.satellites  = v[5].toInt();
            s.hdop        = v[6].toFloat();
            s.Roll        = v[7].toFloat();
            s.Pitch       = v[8].toFloat();
            s.Yaw         = v[9].toFloat();
            s.Servo1      = v[10].toFloat();
            s.Servo2      = v[11].toFloat();
            s.Servo3      = v[12].toFloat();
            s.Servo4      = v[13].toFloat();
            s.AltDiff     = v[14].toFloat();
        }
        if (v.size() >= 17) {
            s.pressure    = v[15].toFloat();
            s.temperature = v[16].toFloat();
        }
        return s;
    }
};

#endif
