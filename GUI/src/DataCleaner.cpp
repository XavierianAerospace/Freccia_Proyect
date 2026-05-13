#include "DataCleaner.h"
#include "FileHelper.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void DataCleaner::clean(std::vector<GPSData>& data) {
    std::vector<GPSData> cleaned;
    const std::string errorFilePath = "../data/errores_gps.csv";
    const std::string cleanFilePath = "../data/clean_gps.csv";
    int errores = 0;
    std::string timestamp = getCurrentTimestamp();

    FileHelper::ensureExists(errorFilePath);
    FileHelper::ensureExists(cleanFilePath);

    std::ofstream errorFile(errorFilePath, std::ios::app);
    if (!errorFile.is_open()) {
        std::cerr << "No se pudo abrir el archivo errores_gps.csv\n";
        return;
    }

    errorFile << "\n------------------Procesamiento: " << timestamp << " ------------------ \n";

    for (const auto& d : data) {
        bool valid = true;

        if (d.latitude < -90 || d.latitude > 90) valid = false;
        if (d.longitude < -180 || d.longitude > 180) valid = false;
        if (d.satellites <= 0) valid = false;
        if (d.hdop <= 0 || d.hdop > 20) valid = false;

        if (valid) {
            cleaned.push_back(d);
            std::cout << "Lat: " << d.latitude << ", Lon: " << d.longitude
                      << ", Fecha: " << d.date << ", Hora: " << d.utc_time
                      << ", Satélites: " << d.satellites << ", HDOP: " << d.hdop << "\n";
        } else {
            ++errores;
            errorFile << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << ","
                      << d.satellites << "," << d.hdop << "\n";
        }
    }

    errorFile << "Número de datos con errores: " << errores << "\n";
    errorFile.close();

    std::cout << "Número de datos con errores: " << errores << "\n";

    std::ofstream cleanFile(cleanFilePath, std::ios::trunc);
    if (!cleanFile.is_open()) {
        std::cerr << "No se pudo abrir el archivo clean_gps.csv\n";
        return;
    }

    cleanFile << "\n------------------Procesamiento Datos: " << timestamp << "------------------ \n";
    cleanFile << "latitude,longitude,date,utc_time,satellites,hdop\n";

    for (const auto& d : cleaned) {
        cleanFile << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << ","
                  << d.satellites << "," << d.hdop << "\n";
    }

    cleanFile.close();
    data = cleaned;
    data.clear();
}