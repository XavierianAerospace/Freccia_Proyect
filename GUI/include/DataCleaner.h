#ifndef DATACLEANER_H
#define DATACLEANER_H

#include "GPSData.h"
#include <vector>

class DataCleaner {
public:
    void clean(std::vector<GPSData>& data);
    void applyMovingAverage(std::vector<GPSData>& data, int windowSize); // No aplica, pero mantenido por estructura
};

#endif
