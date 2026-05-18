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
        QStringList fields;
        fields << QString::number(latitude, 'f', 8);
        fields << QString::number(longitude, 'f', 8);
        fields << QString::fromStdString(date);
        fields << QString::fromStdString(utc_time);
        fields << QString::number(secs, 'f', 2);
        fields << QString::number(satellites);
        fields << QString::number(hdop, 'f', 2);
        fields << QString::number(Roll, 'f', 2);
        fields << QString::number(Pitch, 'f', 2);
        fields << QString::number(Yaw, 'f', 2);
        fields << QString::number(Servo1, 'f', 2);
        fields << QString::number(Servo2, 'f', 2);
        fields << QString::number(Servo3, 'f', 2);
        fields << QString::number(Servo4, 'f', 2);
        fields << QString::number(AltDiff, 'f', 2);
        fields << QString::number(pressure, 'f', 2);
        fields << QString::number(temperature, 'f', 2);
        return fields.join(",");
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
