// DataCleaner.cpp
#include "DataCleaner.h"
#include "FileHelper.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <cmath>
#include <fstream>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void DataCleaner::clean(std::vector<SensorData>& data) {
    const std::string folder    = "../data";
    const std::string cleanFile = folder + "/clean_sensores.csv";
    const std::string errorFile = folder + "/errores_sensores.csv";

    FileHelper::createDataDirectoryIfNeeded();

    const std::string header =
        "Latitud,Longitud,Fecha,HoraUTC,Segundos,Satélites,HDOP,Roll,Pitch,Yaw,"
        "Servo1,Servo2,Servo3,Servo4,AltDiff,Presion,Temperatura";

    FileHelper::ensureExists(cleanFile);
    FileHelper::ensureExists(errorFile);
    FileHelper::writeHeaderIfNew(cleanFile, header);
    FileHelper::writeHeaderIfNew(errorFile,
        "Latitud,Longitud,Fecha,HoraUTC,Satélites,HDOP,ERROR");

    std::string timestamp = getCurrentTimestamp();
    std::ofstream errStream(errorFile, std::ios::app);
    std::ofstream cleanStream(cleanFile, std::ios::app);

    errStream << "\n---- Procesamiento: " << timestamp << " ----\n";
    cleanStream << "\n---- Procesamiento: " << timestamp << " ----\n";

    int errores = 0;

    for (auto& d : data) {
        // ❌ Ya NO escribimos en raw_data.csv aquí (se hace en SensorManager, por evento)

        std::string errorDetail;
        if (!isValid(d, errorDetail)) {
            errores++;
            FileHelper::appendErrorData(errorFile, d, errorDetail);
        }

        correctIfNeeded(d);
        approximateZeroOrInvalids(d);

        FileHelper::appendCleanData(cleanFile, {d});
    }

    std::cout << "Datos procesados. Errores encontrados: " << errores << "\n";

    cleanStream.close();
    errStream.close();
}

// === VALIDACIÓN DE DATOS ===
bool DataCleaner::isValid(const SensorData& d, std::string& errorDetail) {
    bool valid = true;

    if (d.latitude < -90 || d.latitude > 90) {
        valid = false;
        errorDetail += "Latitud ";
    }
    if (d.longitude < -180 || d.longitude > 180) {
        valid = false;
        errorDetail += "Longitud ";
    }
    if (d.satellites <= 0) {
        valid = false;
        errorDetail += "Satélites ";
    }
    if (d.hdop <= 0 || d.hdop > 20) {
        valid = false;
        errorDetail += "HDOP ";
    }

    return valid;
}

// === CORRECCIÓN INTELIGENTE ===
void DataCleaner::correctIfNeeded(SensorData& d) {
    // GPS
    if (d.latitude < -90 || d.latitude > 90) d.latitude = 0.0;
    if (d.longitude < -180 || d.longitude > 180) d.longitude = 0.0;

    // Satélites
    if (d.satellites <= 0 || d.satellites > 100) d.satellites = 4;

    // HDOP
    if (d.hdop <= 0 || d.hdop > 20) d.hdop = 1.5;

    // Ángulos
    if (std::abs(d.Roll) > 360) d.Roll = 0.0f;
    if (std::abs(d.Pitch) > 360) d.Pitch = 0.0f;
    if (std::abs(d.Yaw) > 360) d.Yaw = 0.0f;

    // Altura relativa
    if (std::abs(d.AltDiff) > 50000) d.AltDiff = 0.0f;

    // Servo
    if (d.Servo1 < 0 || d.Servo1 > 180) d.Servo1 = 90.0f;
    if (d.Servo2 < 0 || d.Servo2 > 180) d.Servo2 = 90.0f;
    if (d.Servo3 < 0 || d.Servo3 > 180) d.Servo3 = 90.0f;
    if (d.Servo4 < 0 || d.Servo4 > 180) d.Servo4 = 90.0f;

    // Sensores
    if (d.pressure <= 0 || d.pressure > 2000) d.pressure = 1013.25f;
    if (d.temperature < -50 || d.temperature > 85) d.temperature = 25.0f;
}

// === APROXIMACIÓN DE VALORES CERCANOS A CERO O ERRÁTICOS ===
void DataCleaner::approximateZeroOrInvalids(SensorData& d) {
    auto isNearZero = [](float val) { return std::abs(val) < 0.001f; };

    if (isNearZero(d.Roll)) d.Roll = 0.0f;
    if (isNearZero(d.Pitch)) d.Pitch = 0.0f;
    if (isNearZero(d.Yaw)) d.Yaw = 0.0f;
    if (isNearZero(d.Servo1)) d.Servo1 = 90.0f;
    if (isNearZero(d.Servo2)) d.Servo2 = 90.0f;
    if (isNearZero(d.Servo3)) d.Servo3 = 90.0f;
    if (isNearZero(d.Servo4)) d.Servo4 = 90.0f;

    if (d.pressure <= 0) d.pressure = 1013.25f; // presión atmosférica promedio
    if (d.temperature < -50 || d.temperature > 85) d.temperature = 25.0f; // temp típica
}
