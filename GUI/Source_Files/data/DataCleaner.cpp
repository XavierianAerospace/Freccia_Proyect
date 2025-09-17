#include "DataCleaner.h"
#include "FileHelper.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <cmath>
#include <fstream>

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void DataCleaner::clean(std::vector<SensorData>& data) {
    // ========= Estado de sesión para CLEAN (persiste mientras corre el proceso) =========
    static bool         sesionAbierta   = false;
    static bool         primerRegistro  = false;
    static double       lastSecs        = -1.0;   
    static FileHelper   fhSesion;                 
    static std::string  fechaPrimeraISO;         
    static std::string  horaPrimeraISO;          

    // ========= Inicialización perezosa del archivo de errores =========
    static bool errorInit = false;
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
    SensorData d = data.back();

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
    approximateZeroOrInvalids(d);

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

// === VALIDACIÓN DE DATOS ===
bool DataCleaner::isValid(const SensorData& d, std::string& errorDetail) {
    bool valid = true;

    if (d.latitude < -90 || d.latitude > 90) {
        valid = false; errorDetail += "Latitud ";
    }
    if (d.longitude < -180 || d.longitude > 180) {
        valid = false; errorDetail += "Longitud ";
    }
    if (d.satellites <= 0) {
        valid = false; errorDetail += "Satélites ";
    }
    if (d.hdop <= 0 || d.hdop > 20) {
        valid = false; errorDetail += "HDOP ";
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
    if (std::abs(d.Roll)  > 360) d.Roll  = 0.0f;
    if (std::abs(d.Pitch) > 360) d.Pitch = 0.0f;
    if (std::abs(d.Yaw)   > 360) d.Yaw   = 0.0f;

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

    if (isNearZero(d.Roll))   d.Roll   = 0.0f;
    if (isNearZero(d.Pitch))  d.Pitch  = 0.0f;
    if (isNearZero(d.Yaw))    d.Yaw    = 0.0f;
    if (isNearZero(d.Servo1)) d.Servo1 = 90.0f;
    if (isNearZero(d.Servo2)) d.Servo2 = 90.0f;
    if (isNearZero(d.Servo3)) d.Servo3 = 90.0f;
    if (isNearZero(d.Servo4)) d.Servo4 = 90.0f;

    if (d.pressure <= 0) d.pressure = 1013.25f; // presión atmosférica promedio
    if (d.temperature < -50 || d.temperature > 85) d.temperature = 25.0f; // temp típica
}