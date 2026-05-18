#include "DataCleaner.h"
#include "FileHelper.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <cmath>
#include <fstream>
#include <numeric>
#include <algorithm>

void DataCleaner::clean(std::vector<SensorData>& data) {
    // ========= Inicialización perezosa del archivo de errores =========
    const std::string errorFile = "../data/errores_sensores.csv";
    if (!errorInit) {
        FileHelper::createDataDirectoryIfNeeded();
        FileHelper::ensureExists(errorFile);
        FileHelper::writeHeaderIfNew(errorFile,
            "Latitud,Longitud,Fecha,HoraCompleta,Satélites,HDOP,ERROR");
        errorInit = true;
    }

    if (data.empty()) return;

    // Procesar SOLO el último dato
    SensorData& d = data.back();

    // ================= VALIDACIÓN / CORRECCIÓN =================
    std::string errorDetail;
    // Obtener hora absoluta del sistema con milis
    auto now       = std::chrono::system_clock::now();
    auto t_c       = std::chrono::system_clock::to_time_t(now);
    auto millis    = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream horaAbs;
    horaAbs << std::put_time(std::localtime(&t_c), "%H:%M:%S")
            << "." << std::setw(3) << std::setfill('0') << millis.count();

    std::stringstream fechaAbs;
    fechaAbs << std::put_time(std::localtime(&t_c), "%Y-%m-%d");

    if (!isValid(d, errorDetail)) {
        FileHelper::appendErrorData(errorFile, d, errorDetail); 
    } else {
        // Guardar también OK con hora absoluta
        std::ofstream file(errorFile, std::ios::app);
        file << d.latitude << "," << d.longitude << ","
             << fechaAbs.str() << "," << horaAbs.str() << ","
             << d.satellites << "," << d.hdop << ",OK\n";
        file.close();
    }

    correctIfNeeded(d);
    statisticalClean(d);
    approximateZeroOrInvalids(d);
    updateHistory(d);

    // ================= MANEJO DE SESIÓN =================
    const bool contadorReiniciado = (lastSecs >= 0.0 && d.secs < lastSecs);

    if (!sesionAbierta || contadorReiniciado) {
        if (sesionAbierta) {
            fhSesion.cerrarSesionLimpios();
        }

        // Guardar fecha/hora de inicio completas
        fechaPrimeraISO  = fechaAbs.str();
        horaPrimeraISO   = horaAbs.str();
        fhSesion.iniciarSesionLimpios(fechaPrimeraISO, horaPrimeraISO);
        primerRegistro   = true;
        sesionAbierta    = true;
        lastSecs         = -1.0; 
    }

    fhSesion.escribirLimpioDuranteSesion(static_cast<double>(d.secs), d, primerRegistro,
                                         fechaPrimeraISO, horaPrimeraISO);
    primerRegistro = false;
    lastSecs = d.secs;
}

bool DataCleaner::isValid(const SensorData& d, std::string& errorDetail) {
    bool valid = true;
    if (d.latitude < -90 || d.latitude > 90) { valid = false; errorDetail += "Latitud "; }
    if (d.longitude < -180 || d.longitude > 180) { valid = false; errorDetail += "Longitud "; }
    if (d.satellites <= 0) { valid = false; errorDetail += "Satélites "; }
    if (d.hdop <= 0 || d.hdop > 20) { valid = false; errorDetail += "HDOP "; }
    return valid;
}

void DataCleaner::correctIfNeeded(SensorData& d) {
    if (d.latitude < -90 || d.latitude > 90) d.latitude = 0.0;
    if (d.longitude < -180 || d.longitude > 180) d.longitude = 0.0;
    if (d.satellites <= 0 || d.satellites > 100) d.satellites = 4;
    if (d.hdop <= 0 || d.hdop > 20) d.hdop = 1.5;
    if (std::abs(d.Roll)  > 360) d.Roll  = 0.0f;
    if (std::abs(d.Pitch) > 360) d.Pitch = 0.0f;
    if (std::abs(d.Yaw)   > 360) d.Yaw   = 0.0f;
    if (std::abs(d.AltDiff) > 50000) d.AltDiff = 0.0f;
    if (d.Servo1 < 0 || d.Servo1 > 180) d.Servo1 = 90.0f;
    if (d.Servo2 < 0 || d.Servo2 > 180) d.Servo2 = 90.0f;
    if (d.Servo3 < 0 || d.Servo3 > 180) d.Servo3 = 90.0f;
    if (d.Servo4 < 0 || d.Servo4 > 180) d.Servo4 = 90.0f;
    if (d.pressure <= 0 || d.pressure > 2000) d.pressure = 1013.25f;
    if (d.temperature < -50 || d.temperature > 85) d.temperature = 25.0f;
}

void DataCleaner::approximateZeroOrInvalids(SensorData& d) {
    auto isNearZero = [](float val) { return std::abs(val) < 0.001f; };
    if (isNearZero(d.Roll))   d.Roll   = 0.0f;
    if (isNearZero(d.Pitch))  d.Pitch  = 0.0f;
    if (isNearZero(d.Yaw))    d.Yaw    = 0.0f;
    if (isNearZero(d.Servo1)) d.Servo1 = 90.0f;
    if (isNearZero(d.Servo2)) d.Servo2 = 90.0f;
    if (isNearZero(d.Servo3)) d.Servo3 = 90.0f;
    if (isNearZero(d.Servo4)) d.Servo4 = 90.0f;
    if (d.pressure <= 0) d.pressure = 1013.25f;
    if (d.temperature < -50 || d.temperature > 85) d.temperature = 25.0f;
}

void DataCleaner::statisticalClean(SensorData& d) {
    auto cleanField = [&](const std::string& name, float& value) {
        auto& history = historyBuffers[name];
        if (history.size() >= 10) {
            double mean = getMean(history);
            double stdDev = getStdDev(history, mean);
            if (stdDev > 0.0001 && std::abs(value - mean) > 3 * stdDev) {
                value = static_cast<float>(mean);
            }
        }
    };

    auto cleanFieldDouble = [&](const std::string& name, double& value) {
        auto& history = historyBuffers[name];
        if (history.size() >= 10) {
            double mean = getMean(history);
            double stdDev = getStdDev(history, mean);
            if (stdDev > 0.0001 && std::abs(value - mean) > 3 * stdDev) {
                value = mean;
            }
        }
    };

    cleanFieldDouble("lat", d.latitude);
    cleanFieldDouble("lon", d.longitude);
    cleanField("roll", d.Roll);
    cleanField("pitch", d.Pitch);
    cleanField("yaw", d.Yaw);
    cleanField("alt", d.AltDiff);
    cleanField("press", d.pressure);
    cleanField("temp", d.temperature);
}

void DataCleaner::updateHistory(const SensorData& d) {
    auto add = [&](const std::string& name, double val) {
        auto& h = historyBuffers[name];
        h.push_back(val);
        if (h.size() > maxHistorySize) h.pop_front();
    };
    add("lat", d.latitude);
    add("lon", d.longitude);
    add("roll", d.Roll);
    add("pitch", d.Pitch);
    add("yaw", d.Yaw);
    add("alt", d.AltDiff);
    add("press", d.pressure);
    add("temp", d.temperature);
}

double DataCleaner::getMean(const std::deque<double>& history) {
    if (history.empty()) return 0.0;
    return std::accumulate(history.begin(), history.end(), 0.0) / history.size();
}

double DataCleaner::getStdDev(const std::deque<double>& history, double mean) {
    if (history.size() < 2) return 0.0;
    double sq_sum = std::accumulate(history.begin(), history.end(), 0.0,
        [mean](double acc, double x) { return acc + (x - mean) * (x - mean); });
    return std::sqrt(sq_sum / history.size());
}
