#ifndef DATACLEANER_H
#define DATACLEANER_H

#include "SensorData.h"
#include <vector>
#include <string>

class DataCleaner {
public:
    void clean(std::vector<SensorData>& data);
private:
    bool isValid(const SensorData& d, std::string& errorDetail);
    void correctIfNeeded(SensorData& d);
    void approximateZeroOrInvalids(SensorData& d);
};

#endif
