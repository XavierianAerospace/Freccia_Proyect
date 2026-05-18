#ifndef DATACLEANER_H
#define DATACLEANER_H

#include "SensorData.h"
#include "FileHelper.h"
#include <vector>
#include <string>
#include <map>
#include <deque>

class DataCleaner {
public:
    void clean(std::vector<SensorData>& data);
    void reset();
private:
    bool isValid(const SensorData& d, std::string& errorDetail);
    void correctIfNeeded(SensorData& d);
    void approximateZeroOrInvalids(SensorData& d);

    // Statistical cleaning
    void statisticalClean(SensorData& d);
    void updateHistory(const SensorData& d);
    double getMean(const std::deque<double>& history);
    double getStdDev(const std::deque<double>& history, double mean);

    // Session state
    bool         sesionAbierta   = false;
    bool         primerRegistro  = false;
    double       lastSecs        = -1.0;
    FileHelper   fhSesion;
    std::string  fechaPrimeraISO;
    std::string  horaPrimeraISO;
    bool         errorInit       = false;

    // History buffers for statistical cleaning
    std::map<std::string, std::deque<double>> historyBuffers;
    const size_t maxHistorySize = 50;
};

#endif
