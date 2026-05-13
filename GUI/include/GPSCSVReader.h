// GPSCSVReader.h
#ifndef GPSCSVREADER_H
#define GPSCSVREADER_H

#include <vector>
#include <string>
#include "GPSData.h"

class GPSCSVReader {
public:
    explicit GPSCSVReader(const std::string& filename);
    std::vector<GPSData> readAll();
private:
    std::string m_filename;
};

#endif
