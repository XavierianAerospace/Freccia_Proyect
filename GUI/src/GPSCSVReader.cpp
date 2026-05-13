// GPSCSVReader.cpp
#include "GPSCSVReader.h"
#include <fstream>
#include <sstream>

GPSCSVReader::GPSCSVReader(const std::string& filename) : m_filename(filename) {}

std::vector<GPSData> GPSCSVReader::readAll() {
    std::vector<GPSData> data;
    std::ifstream file(m_filename);
    if (!file.is_open()) return data;

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line)) {
        if (firstLine) {
            firstLine = false;
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> values;

        while (std::getline(ss, token, ',')) {
            values.push_back(token);
        }

        if (values.size() != 6) continue;

        try {
            GPSData gps;
            gps.latitude = std::stod(values[0]);
            gps.longitude = std::stod(values[1]);
            gps.date = values[2];
            gps.utc_time = values[3];
            gps.satellites = std::stoi(values[4]);
            gps.hdop = std::stof(values[5]);

            data.push_back(gps);
        } catch (...) {
            continue;
        }
    }

    return data;
}
